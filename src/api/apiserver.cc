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

    Apimsg_decoder* api_decoder = new Apimsg_decoder();
    Apimsg_user_constraint* user_cons = api_decoder->test_decode_msg(apiMsg->body);
    TLV* tlv_ptr = (TLV*)(new u_int8_t[TLV_HEAD_SIZE + sizeof(user_cons)]);
    tlv_ptr->type = 1;
    tlv_ptr->length = sizeof(user_cons);
    memcpy(tlv_ptr->value, &user_cons, sizeof(user_cons));
    

    string queueName="CORE";
    string topicName="API_REQUEST";
    Message* tMsg = new Message(MSG_REQ, queueName, topicName);
    tMsg->AddTLV(tlv_ptr);
    apiThread->GetMessagePort()->PostMessage(tMsg);

    if (user_cons->getGri().empty())
    {
        char cstrClientConnId[32];
        snprintf(cstrClientConnId, "%d-%d", ntohl(apiMsg->header.ucid), ntohl(apiMsg->header.seqnum));
        string gri = cstrClientConnId;
        user_cons->setGri(gri);
    }
    apiClientConns[user_cons->getGri()] = apiReader;

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
    int msg_body_len = 0;
    while ((msg = msgPort->GetMessage()) != NULL)
    {
        msg->LogDump();

        api_msg* apiMsg = new api_msg();
        Apimsg_encoder* api_encoder = new Apimsg_encoder();
        msg_body_len = api_encoder->test_encode_msg(msg,apiMsg->body);
        api_encoder->encode_msg_header(apiMsg->header, msg_body_len);

        //@@@@ Example code
        //$$ get reply object from msg 
        //$$ encode into api_msg below
        /*
                api_msg_header* apiMsgHeader = &(apiMsg->header);

                apiMsgHeader->type = htons(API_MSG_REPLY);
                apiMsgHeader->ucid = htonl(0x0002);
                apiMsgHeader->length = htons(0);
                apiMsgHeader->seqnum = htonl(0);
                apiMsgHeader->chksum = htonl(MSG_CHKSUM(apiMsg->header));
                apiMsgHeader->options = htonl(0);
                apiMsgHeader->tag = htonl(0);
              */
        //$$ get gri from the reply object
        //$$ map<string, APIReader*, strcmpless>::iterator itClientConn = apiClientConns.find(gri);
        //$$ APIReader* clientConn = (itClientConn !=  apiClientConns.end() ?  apiClientConns[gri] : NULL);
        //$$ if (!clientConn || !clientConn->GetWriter()) 
        //$$          throw exception ...
        //$$ apiConn->GetWriter()->PostMessage(apiMsg);
        //$$ apiClientConns.erase(itClientConn);

        //delete msg; //msg consumed ?
    }
}

