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
#include "mxtce.hh"

// Thread specific logic
void* TEDBManThread::hookRun()
{
    msgPort->SetThreadScheduler(this);

    // thread specific init
    string tedbName = "Master-TEDB";
    tedb = new TEDB(tedbName);
    xmlImporter = new TopologyXMLImporter(tedb, MxTCE::xmlTopoFilePath, 60);
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
            this->GeMessagePort()->PostMessage(fwdMsg);
        }
        delete msg; //msg consumed
    }
}

