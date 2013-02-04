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
#include "mxtce.hh"

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

    Apireqmsg_decoder* api_decoder = new Apireqmsg_decoder();
    Apimsg_user_constraint* user_cons = api_decoder->test_decode_msg(apiMsg->body, ntohs(apiMsg->header.length));
    if (user_cons->getGri().empty())
    {
        char cstrClientConnId[32];
        snprintf(cstrClientConnId, 32, "%d-%d", ntohl(apiMsg->header.ucid), ntohl(apiMsg->header.seqnum));
        string gri = cstrClientConnId;
        user_cons->setGri(gri);
    }
    if (user_cons->getFlexSchedules() == NULL && user_cons->getStarttime() > 0 && user_cons->getEndtime() > user_cons->getStarttime())
    {
        list<TSchedule*>* fs = new list<TSchedule*>;
        TSchedule* sched = new TSchedule((time_t)user_cons->getStarttime(), (time_t)user_cons->getEndtime());
        fs->push_back(sched);
        user_cons->setFlexSchedules(fs);
    }
    string queueName="CORE";
    string topicName="API_REQUEST";
    Message* tMsg = NULL;
    TLV* tlv_ptr = NULL;
    
    if (((ntohl(apiMsg->header.options) & API_OPT_GROUP)) == 0) // none group
    {
        tlv_ptr = (TLV*)(new u_int8_t[TLV_HEAD_SIZE + sizeof(user_cons)]);
        tlv_ptr->type = MSG_TLV_VOID_PTR;
        tlv_ptr->length = sizeof(user_cons);
        memcpy(tlv_ptr->value, &user_cons, sizeof(user_cons));
        tMsg = new Message(MSG_REQ, queueName, topicName);
        tMsg->AddTLV(tlv_ptr);
        apiThread->GetMessagePort()->PostMessage(tMsg);
        apiClientConns[user_cons->getGri()] = apiReader;
        return 0;
    }

    this->AddGroup(user_cons);
    if (((ntohl(apiMsg->header.options) & API_OPT_GROUP_LAST)) != 0) // complete group
    {
        tMsg = new Message(MSG_REQ, queueName, topicName);
        list<Apimsg_user_constraint*>* userConsGroup = this->GetGroup(user_cons->getGri());
        for (list<Apimsg_user_constraint*>::iterator it  = userConsGroup->begin(); it != userConsGroup->end(); it++)
        {
             user_cons = *it;
             tlv_ptr = (TLV*)(new u_int8_t[TLV_HEAD_SIZE + sizeof(user_cons)]);
             tlv_ptr->type = MSG_TLV_VOID_PTR;
             tlv_ptr->length = sizeof(user_cons);
             memcpy(tlv_ptr->value, &user_cons, sizeof(user_cons));
             tMsg->AddTLV(tlv_ptr);
        }
        this->DeleteGroup(user_cons->getGri());
        apiThread->GetMessagePort()->PostMessage(tMsg);
        apiClientConns[user_cons->getGri()] = apiReader;
    }
    return 0;
}

void MxTCEAPIServer::AddGroup(Apimsg_user_constraint* userCons)
{
    map<string, list<Apimsg_user_constraint*>*, strcmpless>::iterator  groupIter = userConsGroupCache.find(userCons->getGri());
    if (groupIter == userConsGroupCache.end()) 
    {
        list<Apimsg_user_constraint*>* group = new list<Apimsg_user_constraint*>;
        group->push_back(userCons);
        userConsGroupCache[userCons->getGri()] = group;
    }
    else
    {
        userConsGroupCache[userCons->getGri()]->push_back(userCons);
    }
}

list<Apimsg_user_constraint*>* MxTCEAPIServer::GetGroup(string& gri)
{
    map<string, list<Apimsg_user_constraint*>*, strcmpless>::iterator  groupIter = userConsGroupCache.find(gri);
    if (groupIter != userConsGroupCache.end()) 
        return userConsGroupCache[gri];
    return NULL;
}

void MxTCEAPIServer::DeleteGroup(string& gri)
{
    map<string, list<Apimsg_user_constraint*>*, strcmpless>::iterator  groupIter = userConsGroupCache.find(gri);
    if (groupIter != userConsGroupCache.end()) 
        userConsGroupCache.erase(groupIter);
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
    string gri;
    char buf[128];
    
    while ((msg = msgPort->GetMessage()) != NULL)
    {
        msg->LogDump();
        map<string, APIReader*, strcmpless>::iterator itClientConn;
        list<TLV*>::iterator it = msg->GetTLVList().begin();
        APIReader* clientConn = NULL;
        for (; it != msg->GetTLVList().end(); it++) 
        {
            TLV* tlv = (*it);
            ComputeResult* result;
            memcpy(&result, tlv->value, sizeof(void*));
            api_msg* apiMsg = new api_msg();
            Apireplymsg_encoder* api_encoder = new Apireplymsg_encoder();
            msg_body_len = api_encoder->test_encode_msg(result,apiMsg->body);
            api_encoder->encode_msg_header(apiMsg->header, msg_body_len);
            if (msg->GetTLVList().size() > 1) 
            {
                apiMsg->header.options |= htonl(API_OPT_GROUP);
                list<TLV*>::iterator last = it;
                last++;
                if (last == msg->GetTLVList().end()) 
                    apiMsg->header.options |= htonl(API_OPT_GROUP_LAST);
            }
            gri = api_encoder->get_gri();
            itClientConn = this->apiServer.apiClientConns.find(gri);
            if (it ==  msg->GetTLVList().begin()) 
            {
                if (itClientConn !=  this->apiServer.apiClientConns.end())
                    clientConn = this->apiServer.apiClientConns[gri];
                if (!clientConn || !clientConn->GetWriter())
                {
                    cout<<"can not get APIReader"<<endl;
                    snprintf(buf, 128, "APIServerThread::hookHandleMessage() can't get APIReader");
                    LOG(buf<<endl);
                    throw APIException(buf);
                }
            }
            clientConn->GetWriter()->PostMessage(apiMsg);
        }
        if (clientConn != NULL) 
        {
            this->apiServer.apiClientConns.erase(itClientConn);
            // delete clientConn ?
        }
         //delete msg; //msg consumed ?
    }
}

