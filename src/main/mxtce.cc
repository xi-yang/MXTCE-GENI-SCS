/*
 * Copyright (c) 2010
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


int MxTCE::apiServerPort = 2089;
string MxTCE::apiServerPortName = "MX-TCE_API_SERVER";
string MxTCE::tedbManPortName = "MX-TCE_TEDB_MANAGER";
string MxTCE::resvManPortName = "MX-TCE_RESV_MANAGER";
string MxTCE::policyManPortName = "MX-TCE_POLICY_MANAGER";
string MxTCE::loopbackPortName = "MX-TCE_CORE_LOOPBACK";
string MxTCE::tmpFilesDir = "/var/tmp/mxtce/pipes/";

MxTCE::MxTCE( const string& configFile) 
{
    // $$$$ read and parse configure file from configFile path

    eventMaster = EventMaster::GetInstance();
    assert(eventMaster);

    messageRouter = MessageRouter::GetInstance();
    assert(messageRouter);
    messageRouter->SetEventMaster(eventMaster);

    loopbackPort = new MessagePortLoopback(loopbackPortName, messageRouter, this);
    loopbackPort->SetEventMaster(eventMaster);

    apiServerThread = new APIServerThread(MxTCE::apiServerPortName, MxTCE::apiServerPort, eventMaster);
}


MxTCE::~MxTCE()
{
    //$$$$ Stop eventMaster
    //$$$$ Stop messageRouter
    //$$$$ Stop loopbackMessagePort
}


void MxTCE::Start()
{
    mkpath((const char*)MxTCE::tmpFilesDir.c_str(), 0777);

    // start binary API server thread 
    apiServerThread->Start(NULL);
    
    // $$$$ start TEDB thread
    // --> attach message router port
    
    // $$$$ start ResvMan thread
    // --> attach message router port
    
    // $$$$ start PolicyMan thread
    // --> attach message router port

    messageRouter->AddPort(MxTCE::apiServerPortName);
    string routeQueue = "CORE", routeTopic = "API_REQUEST";
    messageRouter->AddRoute(routeQueue,routeTopic, MxTCE::loopbackPortName);
    routeTopic = "API_REPLY";
    messageRouter->AddRoute(routeQueue,routeTopic, MxTCE::apiServerPortName);
    
    //messageRouter->AddPort(MxTCE::tedbManPortName);
    //messageRouter->AddPort(MxTCE::resvManPortName);
    //messageRouter->AddPort(MxTCE::policyManPortName);
    messageRouter->GetMessagePortList().push_back(loopbackPort);
    loopbackPort->AttachPipesAsServer();
    messageRouter->Start();

    // run core eventMaster
    eventMaster->Run();

    // join threads
    apiServerThread->Join();
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
        // @@@@ Testing code
        string topic = "API_REPLY";
        msg->SetTopic(topic);
        mxTCE->GetLoopbackPort()->PostLocalMessage(msg);
    }
}
