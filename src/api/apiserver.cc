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
    string queueName="CORE";
    string topicName="API_REQUEST";
    Message* tMsg = NULL;
    TLV* tlv_ptr = NULL;

    // temp test code
    if (MxTCE::tempTest)
    {
        tMsg = new Message(MSG_REQ, queueName, topicName);
        Apimsg_user_constraint* userCons = new Apimsg_user_constraint[3];
        string ep1 = "urn:ogf:network:domain=ion.internet2.edu:node=rtr.chic:port=xe-2/1/0:link=*";
        string ep2 = "urn:ogf:network:domain=ion.internet2.edu:node=rtr.newy:port=xe-1/0/1:link=*";
        string ep3 = "urn:ogf:network:domain=es.net:node=bnl-mr3:port=xe-1/2/0:link=*";
        string vtag = "any";
        
        time_t starttime = time(0); 
        time_t endtime = starttime + 10800;
        int duration = 3600;
        TSchedule* ts1 = new TSchedule(starttime, endtime, duration);
        list<TSchedule*>* tsList = new list<TSchedule*>;
        tsList->push_back(ts1);

        userCons[0].setGri("test1");
        userCons[0].setPathId("test1-path1");
        userCons[0].setSrcendpoint(ep1);
        userCons[0].setSrcvlantag(vtag);
        userCons[0].setDestendpoint(ep2);
        userCons[0].setDestvlantag(vtag);
        userCons[0].setStarttime(0);
        userCons[0].setEndtime(0);
        userCons[0].setBandwidth(500000000);
        userCons[0].setFlexMaxBandwidth(1000000000);
        userCons[0].setFlexMinBandwidth(200000000);
        userCons[0].setFlexGranularity(100000000);
        userCons[0].setFlexSchedules(tsList);

        Apimsg_stornet_constraint* coscheduleOptConstraint = new Apimsg_stornet_constraint;
        coscheduleOptConstraint->setStarttime(1335282854);
        coscheduleOptConstraint->setEndtime(1335287000);
        coscheduleOptConstraint->setBandwidthavaigraph(true);
        coscheduleOptConstraint->setMaxnumofaltpaths(3);
        coscheduleOptConstraint->setRequireLinkBag(true);
        coscheduleOptConstraint->setMinbandwidth(1);
        userCons[0].setCoschedreq(coscheduleOptConstraint);

        userCons[1].setGri("test1");
        userCons[1].setPathId("test1-path2");
        userCons[1].setSrcendpoint(ep1);
        userCons[1].setSrcvlantag(vtag);
        userCons[1].setDestendpoint(ep3);
        userCons[1].setDestvlantag(vtag);
        userCons[1].setStarttime(0);
        userCons[1].setEndtime(0);
        userCons[1].setBandwidth(500000000);
        userCons[1].setFlexMaxBandwidth(1000000000);
        userCons[1].setFlexMinBandwidth(200000000);
        userCons[1].setFlexGranularity(100000000);
        userCons[1].setFlexSchedules(tsList);

        userCons[2].setGri("test1");
        userCons[2].setPathId("test1-path3");
        userCons[2].setSrcendpoint(ep2);
        userCons[2].setSrcvlantag(vtag);
        userCons[2].setDestendpoint(ep3);
        userCons[2].setDestvlantag(vtag);
        userCons[2].setStarttime(0);
        userCons[2].setEndtime(0);
        userCons[2].setBandwidth(500000000);
        userCons[2].setFlexMaxBandwidth(1000000000);
        userCons[2].setFlexMinBandwidth(200000000);
        userCons[2].setFlexGranularity(100000000);
        userCons[2].setFlexSchedules(tsList);

        for (int i = 0; i < 3; i++)
        {
            void * ptr = userCons+i;
            tlv_ptr = (TLV*)(new u_int8_t[TLV_HEAD_SIZE + sizeof(ptr)]);
            tlv_ptr->type = MSG_TLV_VOID_PTR;
            tlv_ptr->length = sizeof(ptr);
            memcpy(tlv_ptr->value, &ptr, sizeof(ptr));
            tMsg->AddTLV(tlv_ptr);
        }
        apiThread->GetMessagePort()->PostMessage(tMsg);
        apiClientConns[userCons[0].getGri()] = apiReader;
        return 0;
    }
    
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

        api_msg* apiMsg = new api_msg();
        Apireplymsg_encoder* api_encoder = new Apireplymsg_encoder();
        msg_body_len = api_encoder->test_encode_msg(msg,apiMsg->body);

        api_encoder->encode_msg_header(apiMsg->header, msg_body_len);
  
        gri = api_encoder->get_gri();
        map<string, APIReader*, strcmpless>::iterator itClientConn = this->apiServer.apiClientConns.find(gri);
        APIReader* clientConn = (itClientConn !=  this->apiServer.apiClientConns.end() ?  this->apiServer.apiClientConns[gri] : NULL);
        if (!clientConn || !clientConn->GetWriter())
        {
        	cout<<"can not get APIReader"<<endl;
            snprintf(buf, 128, "APIServerThread::hookHandleMessage() can't get APIReader");
            LOG(buf<<endl);
            throw APIException(buf);
        }
         clientConn->GetWriter()->PostMessage(apiMsg);
         this->apiServer.apiClientConns.erase(itClientConn);
         //delete msg; //msg consumed ?
    }
}

