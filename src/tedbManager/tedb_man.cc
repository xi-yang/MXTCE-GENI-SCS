/*
 * Copyright (c) 2010-2011
 * ARCHSTONE Project.
 * University of Southern California/Information Sciences Institute.
 * All rights reserved.
 *
 * Created by Xi Yang 2011
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "resource.hh"
#include "tedb_man.hh"
#include "resv_apiserver.hh"
#include "mxtce.hh"

// Thread specific logic
void* TEDBManThread::hookRun()
{
    msgPort->SetThreadScheduler(this);

    // thread specific init
    string tedbName = "Master-TEDB";
    tedb = new TEDB(tedbName);
    xmlImporter = new TopologyXMLImporter(tedb, MxTCE::xmlDomainFileList, 60);
    xmlImporter->Run();
    eventMaster->Schedule(xmlImporter);
    tedb->LogDump();
    // start event loop. eventMaster has been initiated in ThreadPortScheduler::Run()
    eventMaster->Run();
}


// Handle message from thread message router
void TEDBManThread::hookHandleMessage()
{
    Message* msg = NULL;
    while ((msg = msgPort->GetMessage()) != NULL)
    {
        msg->LogDump();
        if (msg->GetTopic() == "TEWG_REQUEST") 
        {
            Message* fwdMsg = msg->Duplicate();
            fwdMsg->SetType(MSG_REQ);
            // get TEWG simply by taking full snapshot copy
            TEWG* tewg = tedb->GetSnapshot(msg->GetQueue());
            string topic = "TEWG_RESV_REQUEST";
            fwdMsg->SetTopic(topic);
            // use the same queue that is dedicated to computeThread
            TLV* tlv = (TLV*)new char[TLV_HEAD_SIZE + sizeof(void*)];
            tlv->type = MSG_TLV_VOID_PTR;
            tlv->length = sizeof(void*);
            memcpy(tlv->value, (void*)&tewg, sizeof(void*)); 
            fwdMsg->AddTLV(tlv);
            this->GetMessagePort()->PostMessage(fwdMsg);
        }
        else if (msg->GetTopic() == "TEDB_ADD_RESV") 
        {
            string domain;
            string gri;
            string status;
            long bw = 0;
            int mtu = 9000;
            time_t start, end;
            list<TLink*> path;

            char buf[256];

            //$$ get reservation info
            list<TLV*>& tlvList = msg->GetTLVList();
            list<TLV*>::iterator itlv = tlvList.begin();
            int seq = 0;
            for (; itlv != tlvList.end(); itlv++)
            {
                if ((*itlv)->type == MSG_TLV_RESV_INFO)
                {
                    TLV_ResvInfo* tlv = (TLV_ResvInfo*)(*itlv);
                    domain = (const char*)tlv->domain;
                    gri = (const char*)tlv->gri;
                    start = tlv->start_time;
                    end = tlv->end_time;
                    bw = tlv->bandwidth;
                    status = (const char*)tlv->status;
                    snprintf(buf, 256, "RESV domain=%s, gri=%s, bw=%ld\n", domain.c_str(), gri.c_str(), bw); 
                    LOG_DEBUG(buf);
                }
                else if ((*itlv)->type == MSG_TLV_PATH_ELEM)
                {
                    ISCD* iscd = NULL;
                    seq++;
                    TLV_PathElem* tlv = (TLV_PathElem*)(*itlv);
                    string urn = (const char*)tlv->urn;
                    TLink* link = new TLink(seq, urn);
                    if (tlv->switching_type == LINK_IFSWCAP_L2SC) {
                        iscd = new ISCD_L2SC(bw, mtu);
                        if (tlv->vlan != 0)
                            ((ISCD_L2SC*)iscd)->assignedVlanTags.AddTag(tlv->vlan);
                    }
                    else if (tlv->switching_type <= LINK_IFSWCAP_PSC4)
                        iscd = new ISCD_PSC((int)tlv->switching_type, bw, mtu);
                    // TODO:  support for TMD and LSC
                    // assert(bw > 0 && iscd != NULL);
                    if (bw <= 0 || iscd == NULL)
                    {
                        snprintf(buf, 256, "Invalid RESV data from domain=%s, gri=%s, bw=%ld\n", domain.c_str(), gri.c_str(), bw); 
                        LOG_DEBUG(buf);
                        continue;
                    }
                    link->GetSwCapDescriptors().push_back(iscd);
                    link->SetMaxBandwidth(bw);
                    link->SetMaxReservableBandwidth(bw);
                    path.push_back(link);
                }                    
            }

            if (gri.length() == 0 || path.size() == 0)
            {
                snprintf(buf, 256, "Invalid RESV data from domain=%s, gri=%s, path.size=%d\n", domain.c_str(), gri.c_str(), path.size()); 
                LOG_DEBUG(buf);
                continue;
            }
            //$$ create TReservation
            TReservation* resv = new TReservation(gri);
            resv->GetSchedules().push_back(new TSchedule(start, end));
            TGraph* serviceTopo = new TGraph(gri);
            try {
                serviceTopo->LoadPath(path); 
            } catch  (TEDBException e) {
                LOG_DEBUG("Invalid RESV data from domain: " << domain << ", gri=" << gri << ", " << e.GetMessage() << endl);
                continue; 
            }
            resv->SetServiceTopology(serviceTopo);
            resv->SetStatus(status);

            //$$ send result reservation back to resvMan
            string queue = "RESV", topic = "TEDB_ADD_RESV_REPLY";
            Message* fwdMsg = new Message(MSG_REQ, queue, topic);
            TLV* tlv = (TLV*)new char[TLV_HEAD_SIZE + sizeof(void*)];
            tlv->type = MSG_TLV_VOID_PTR;
            tlv->length = sizeof(void*);
            memcpy(tlv->value, (void*)&resv, sizeof(void*)); 
            fwdMsg->AddTLV(tlv);
            this->GetMessagePort()->PostMessage(fwdMsg);
        }
        delete msg; //msg consumed
    }
}

