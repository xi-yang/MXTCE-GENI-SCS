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

#include "log.hh"
#include "event.hh"
#include "exception.hh"
#include "utils.hh"
#include "mxtce.hh"
#include "mxtce_config.hh"
#include "compute_worker.hh"


int MxTCE::apiServerPort = 2089;
int MxTCE::xmlrpcApiServerPort = 8081;
int MxTCE::resvApiServerPort = 2091;
string MxTCE::apiServerPortName = "MX-TCE_API_SERVER";
string MxTCE::xmlrpcApiServerPortName = "MX-TCE_XMLRPC_API_SERVER";
string MxTCE::tedbManPortName = "MX-TCE_TEDB_MANAGER";
string MxTCE::resvManPortName = "MX-TCE_RESV_MANAGER";
string MxTCE::policyManPortName = "MX-TCE_POLICY_MANAGER";
string MxTCE::loopbackPortName = "MX-TCE_CORE_LOOPBACK";
string MxTCE::computeThreadPrefix = "COMPUTE_THREAD_";
string MxTCE::defaultComputeWorkerType = "simpleComputeWorker";
list<string> MxTCE::xmlDomainFileList;
bool MxTCE::tempTest = false;

MxTCE::MxTCE( const string& configFile) 
{
    configParser = new MxTCEConfig(this, configFile);

    eventMaster = new EventMaster;
    assert(eventMaster);

    messageRouter = MessageRouter::GetInstance();
    assert(messageRouter);
    messageRouter->SetEventMaster(eventMaster);

    loopbackPort = new MessagePortLoopback(loopbackPortName, messageRouter, this);
    loopbackPort->SetEventMaster(eventMaster);

    apiServerThread = new APIServerThread(MxTCE::apiServerPortName, MxTCE::apiServerPort);
    xmlrpcApiServerThread = new XMLRPC_APIServer(this);
    tedbManThread = new TEDBManThread(MxTCE::tedbManPortName);
    resvManThread = new ResvManThread(MxTCE::resvManPortName);
    policyManThread = new PolicyManThread(MxTCE::policyManPortName);
}


MxTCE::~MxTCE()
{
    //$$$$ Stop eventMaster
    //$$$$ Stop messageRouter
    //$$$$ Stop loopbackMessagePort
}


void MxTCE::Start()
{
    // exception could be thrown here and program will abort as expected
    configParser->ParseYamlConfig();
        
    // init binary apiServer port and routes on messge router
    messageRouter->AddPort(MxTCE::apiServerPortName);
    
    // TODO: add xmlrpc ApiServer port on-demand by xmlrpc call
    //messageRouter->AddPort(MxTCE::xmlrpcApiServerPortName);
	//routeTopic1 = "XMLRPC_API_REQUEST", routeTopic2 = "XMLRPC_API_REPLY";
    //messageRouter->AddRoute(routeQueue,routeTopic1, MxTCE::loopbackPortName);
    //messageRouter->AddRoute(routeQueue,routeTopic2, MxTCE::xmlrpcApiServerPortName);

    // init TEDBMan port and routes on messge router
    messageRouter->AddPort(MxTCE::tedbManPortName);

    // init ResvMan port and routes on messge router
    messageRouter->AddPort(MxTCE::resvManPortName);

    // init PolicyMan port and routes on messge router
    messageRouter->AddPort(MxTCE::policyManPortName);

    // init core loopback message port on messge router
    messageRouter->GetMessagePortList().push_back(loopbackPort);
    loopbackPort->AttachPipes();

    // start binary API server thread 
    apiServerThread->Start(NULL);

    // start xmlrpc API server thread 
    xmlrpcApiServerThread->Start(NULL);
    
    // start TEDB thread
    tedbManThread->Start(NULL);
    
    // start ResvMan thread
    resvManThread->Start(NULL);
    
    // start PolicyMan thread
    policyManThread->Start(NULL);

    // core workflow message routes
    string routeQueue = "CORE", routeTopic1 = "API_REQUEST", routeTopic2 = "API_REPLY";
    messageRouter->AddRoute(routeQueue,routeTopic1, MxTCE::loopbackPortName);
    messageRouter->AddRoute(routeQueue,routeTopic2, MxTCE::apiServerPortName);
    routeQueue = "RESV"; routeTopic1 = "TEDB_ADD_RESV", routeTopic2 = "TEDB_ADD_RESV_REPLY";
    messageRouter->AddRoute(routeQueue,routeTopic1, MxTCE::tedbManPortName);
    messageRouter->AddRoute(routeQueue,routeTopic2, MxTCE::resvManPortName);

    // start message router
    messageRouter->Start();

    // run core eventMaster
    eventMaster->Run();

    // join threads
    apiServerThread->Join();
	xmlrpcApiServerThread->Join();
    tedbManThread->Join();
    resvManThread->Join();
    policyManThread->Join();
}


void MxTCE::CheckMessage()
{
    MxTCEMessageHandler* msgHandler = new MxTCEMessageHandler(this);
    msgHandler->SetRepeats(0);
    msgHandler->SetAutoDelete(true);
    msgHandler->SetObsolete(false);
    eventMaster->Schedule(msgHandler);
}


//////////////////////////////////////

// $$$$ TCE workflow
//  --> event driven handling for requests/replies
void MxTCEMessageHandler::Run()
{
    LOG("MxTCEMessageHandler::Run() being called"<<endl);

    Message* msg = mxTCE->GetLoopbackPort()->GetLocalMessage();

    if (msg)
    {
        msg->LogDump();
        if (msg->GetType() == MSG_REQ && msg->GetTopic() == "API_REQUEST") {
            // creating computeWorkerThread and pass user request parameters
            ComputeWorker* computingThread = ComputeWorkerFactory::CreateComputeWorker(MxTCE::defaultComputeWorkerType);
            if (msg->GetTLVList().size() > 0) 
            {
                Apimsg_user_constraint* userConstraint;
                list<Apimsg_user_constraint*>* userConsList = new list<Apimsg_user_constraint*>;
                for (list<TLV*>::iterator it = msg->GetTLVList().begin(); it != msg->GetTLVList().end(); it++)
                {
                    memcpy(&userConstraint, (*it)->value, sizeof(void*));
                    userConsList->push_back(userConstraint);
                }
                computingThread->SetWorkflowData("USER_CONSTRAINT_LIST", userConsList);
            }
            else 
            {
                char buf[128];
                snprintf(buf, 128, "Empty TLV list (USER_CONSTRAINT_LIST) in API_REQUEST message from : %s", msg->GetPort()->GetName().c_str());
                throw TCEException(buf);
            }

            // init computing thread port and routes on messge router and start thread
            string computeThreadPortName = computingThread->GetName();
            mxTCE->GetMessageRouter()->AddPort(computeThreadPortName);
            string computeThreadQueueName = MxTCE::computeThreadPrefix + computingThread->GetName();
            string routeTopic1 = "COMPUTE_REQUEST", routeTopic2 = "COMPUTE_REPLY",
                routeTopic3 = "TEWG_REQUEST", routeTopic4 = "TEWG_RESV_REQUEST", routeTopic5 = "TEWG_REPLY", 
                routeTopic6 = "POLICY_REQUEST", routeTopic7 = "POLICY_REPLY";

            // coreThread send compute request to computeThread
            mxTCE->GetMessageRouter()->AddRoute(computeThreadQueueName,routeTopic1, computeThreadPortName);
            // computeThread reply to coreThread with computation result
            mxTCE->GetMessageRouter()->AddRoute(computeThreadQueueName,routeTopic2, MxTCE::loopbackPortName);

            // computeThread send TEWG request to tedbMan
            mxTCE->GetMessageRouter()->AddRoute(computeThreadQueueName,routeTopic3, MxTCE::tedbManPortName);
            // tedbMan forward the request with TEWG to resvMan
            mxTCE->GetMessageRouter()->AddRoute(computeThreadQueueName,routeTopic4, MxTCE::resvManPortName);
            // resvMan reply to computeThread with TEWG and added deltaList
            mxTCE->GetMessageRouter()->AddRoute(computeThreadQueueName,routeTopic5, computeThreadPortName);
            // TODO: policyMan can be invloved in the above TEWG chain and add policy modifiers to TEWG
            mxTCE->GetMessageRouter()->AddRoute(computeThreadQueueName,routeTopic6, MxTCE::policyManPortName);
            mxTCE->GetMessageRouter()->AddRoute(computeThreadQueueName,routeTopic7, computeThreadPortName);

            // start compute thread
            computingThread->Start(NULL);

            // pass the workflow init message with request details to computingThread
            Message* msg_compute_request = msg->Duplicate();
            msg_compute_request->SetQueue(computeThreadQueueName);
            msg_compute_request->SetTopic(routeTopic1);
            mxTCE->GetLoopbackPort()->PostLocalMessage(msg_compute_request);
        } 
        else if (msg->GetType() == MSG_REPLY && msg->GetTopic() == "COMPUTE_REPLY") 
        {
            string queue = "CORE";
            string topic = "API_REPLY";
            Message* msg_reply = msg->Duplicate();
            msg_reply->SetType(MSG_REPLY);
            msg_reply->SetQueue(queue);
            msg_reply->SetTopic(topic);
            mxTCE->GetLoopbackPort()->PostLocalMessage(msg_reply);

            // find and join the computeWorker thread ? 
            ComputeWorker* computingThread = ComputeWorkerFactory::LookupComputeWorker(msg->GetPort()->GetName());
            if (computingThread == NULL)
            {
                char buf[128];
                snprintf(buf, 128, "Unknown computeWorkerThread: %s", msg->GetPort()->GetName().c_str());
                throw TCEException(buf);
            }
            //TODO: fix dup free by DeletePort
            mxTCE->GetMessageRouter()->DeletePort(computingThread->GetName());
            computingThread->GetEventMaster()->Stop();
            computingThread->Join();
        }
		// TODO: handle XMLRPC_API_REQUEST and XMLRPC_API_REPLY
    }

    delete msg; //msg consumed
}
