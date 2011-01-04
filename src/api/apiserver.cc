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

#include "apiserver.hh"

// Handle message from API client

// TODO: Exception handling!
int MxTCEAPIServer::HandleAPIMessage (APIReader* apiReader, APIWriter* apiWriter, api_msg * apiMsg)
{
    
    if (ntohs(apiMsg->header.type) != API_MSG_REQUEST)
    {
        LOGF("MxTCEAPIServer::HandleMessage:: The impossible happened:  Received a non-API_MSG_REQUEST message type: %d (ucid=0x%x, seqno=0x%x).\n",
            ntohs(apiMsg->header.type), ntohl(apiMsg->header.ucid), ntohl(apiMsg->header.seqnum));
        return -1;
    }


    // @@@@ Testing code -- Begin

    apiMsg->header.type = htons(API_MSG_REPLY);
    apiMsg->header.ucid = htonl(0x0002);
    apiMsg->header.chksum = MSG_CHKSUM(apiMsg->header);
    apiWriter->PostMessage(apiMsg);

    // use macro defs for queue and topic names??
    string queueName="CORE", topicName="API_REQUEST";
    Message* tMsg = new Message(MSG_REQ, queueName, topicName);
    // empty msg body!
    // tMsg->AddTLV();
    apiThread->GetMessagePort()->PostMessage(tMsg); 

    // @@@@ Testing code -- End

    
    // Extract TLVs --> validate request, translate info and compose message to pass to core
    /*
    switch (ntohs(app_req->type))
    {
    case TLV_TYPE_NARB_REQUEST:
        {
        }
        break;
    default:
        LOGF("Unknow APP->NARB message type (%d)\n", ntohs(app_req->type));
    }
    */     
    return 0;
}


// Thread specific logic
void* APIServerThread::hookRun()
{
    msgPort->SetThreadScheduler(this);
    // eventMaster has been initiated in parent class ThreadPortScheduler::Run() method
    apiServer.SetEventMaster(eventMaster);
    apiServer.Start();
    eventMaster->Run();
}


// Handle message from thread message router
void APIServerThread::hookHandleMessage()
{
    Message* msg = NULL;
    while ((msg = msgPort->GetMessage()) != NULL)
    {
        msg->LogDump();
    }
}

