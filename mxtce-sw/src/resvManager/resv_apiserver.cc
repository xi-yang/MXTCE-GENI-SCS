/*
 * Copyright (c) 2010-2011
 * ARCHSTONE Project.
 * University of Southern California/Information Sciences Institute.
 * All rights reserved.
 *
 * Created by Xi Yang 2010
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

#include "resv_apiserver.hh"
#include "resv_man.hh"

// Handle message from API client

// TODO: Exception handling!
int ResvAPIServer::HandleAPIMessage (APIReader* apiReader, APIWriter* apiWriter, api_msg * apiMsg)
{
    if (ntohs(apiMsg->header.type) != API_MSG_RESV_PUSH)
    {
        LOGF("ResvAPIServer::HandleMessage:: The impossible happened:  Received a non-API_MSG_RESV_PUSH message type: %d (ucid=0x%x, seqno=0x%x).\n",
            ntohs(apiMsg->header.type), ntohl(apiMsg->header.ucid), ntohl(apiMsg->header.seqnum));
        return -1;
    }

    TLV_ResvInfo* tlvResvInfo = (TLV_ResvInfo*)(apiMsg->body);
    string gri = tlvResvInfo->gri;
    string domain = tlvResvInfo->domain;
    TReservation* theResv = resvManThread->GetRData()->LookupReservationInDomain(gri, domain);

    string status = "DELETE";
    if ((ntohl(apiMsg->header.options) & API_MSG_OPT_RESV_BURSTHEAD) != 0)
    {
        //start of update burst --> mark all reservations in the domain as removal candidates
        list<TReservation*>* domainResvs = resvManThread->GetRData()->GetReservationsInDomain(domain);
        list<TReservation*>::iterator itR = domainResvs->begin();
        for (; itR != domainResvs->end(); itR++)
            (*itR)->SetStatus(status);
        delete domainResvs;
    }
    // update status of the current reservation
    status = tlvResvInfo->status;
    if (theResv != NULL)
    {
        theResv->SetStatus(status);
    }
    // erase all reservations that are marked as 'DELETE' at the end of burst
    if ((ntohl(apiMsg->header.options) & API_MSG_OPT_RESV_BURSTLAST) != 0)
    {
        //end of update burst --> remove all removal candidates
        list<TReservation*>& resvs = resvManThread->GetRData()->GetReservations();
        list<TReservation*>::iterator itR = resvs.begin();
        for (; itR != resvs.end(); itR++)
        {
            if ((*itR)->GetStatus() == "DELETE")
            {
                delete (*itR);
                itR = resvs.erase(itR);
            }
        }
    }
    // delete apiMsg and return 0 for existing and continuing reservation
    if (theResv != NULL)
    {
        delete apiMsg;
        return 0;
    }

    // each api message contains a single reservation that should contain 
    // tlv_resv_info(gri, bandwidth, schedule) and list of tlv_path_info (urn, swcap, vlan)
    string queueName="RESV";
    string topicName="TEDB_ADD_RESV";
    Message* tedbMsg = new Message(MSG_REQ, queueName, topicName);
    int msglen = ntohs(apiMsg->header.length);
    int offset = 0;
    while (offset < msglen)
    {
        TLV* tlv = (TLV*)(apiMsg->body + offset);
        u_int16_t tlvType = ntohs(tlv->type);
        u_int16_t tlvLength = ntohs(tlv->length);
        TLV* newTlv = (TLV*) new char[TLV_HEAD_SIZE+tlvLength];
        memcpy((char*)newTlv, apiMsg->body + offset, TLV_HEAD_SIZE+tlvLength);
        newTlv->type = tlvType;
        newTlv->length = tlvLength;
        tedbMsg->AddTLV(newTlv);
        offset += (TLV_HEAD_SIZE+tlvLength);
    }
    delete  apiMsg; //apiMsg consumed
    resvManThread->GetMessagePort()->PostMessage(tedbMsg);
    return 0;
}

