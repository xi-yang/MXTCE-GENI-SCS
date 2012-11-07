/*
 * Copyright (c) 2012
 * GENI Project
 * University of Maryland/Mid-Atlantic Crossroads.
 * All rights reserved.
 *
 * Created by Xi Yang 2012
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


#include "xmlrpc_apiserver.hh"
#include "mxtce.hh"
#include <map>

// TODO: Exception handling!


// Base class 
// TODO: move msgPort to thread level (or make class static) if more than one nethod
void XMLRPC_BaseMethod::init() 
{
    mxTCE->GetMessageRouter()->AddPort(MxTCE::xmlrpcApiServerPortName);
    string routeQueue = "CORE", routeTopic1 = "XMLRPC_API_REQUEST", routeTopic2 = "XMLRPC_API_REPLY";
    mxTCE->GetMessageRouter()->AddRoute(routeQueue,routeTopic1, MxTCE::loopbackPortName);
    mxTCE->GetMessageRouter()->AddRoute(routeQueue,routeTopic2, MxTCE::xmlrpcApiServerPortName);
    msgPort = MessagePipeFactory::LookupMessagePipe(MxTCE::xmlrpcApiServerPortName)->GetClientPort();
    assert(msgPort);
    evtMaster = new EventMaster;
    msgPort->SetEventMaster(evtMaster);
    msgPort->SetThreadScheduler(NULL);
    if (!msgPort->IsUp())
    {
        try {
            msgPort->AttachPipes();
        } catch (MsgIOException& e) {
            LOG("XMLRPC_APIServer::Run caugh Exception: " << e.what() << endl);
        }
    }
}

void XMLRPC_BaseMethod::fire() 
{
    // TODO: use callback to stop eventmaster (need to modify MessagePort to hook up extra callback)
    XMLRPC_TimeoutTimer* timeoutTimer = new XMLRPC_TimeoutTimer(evtMaster);
    assert(evtMaster);
    evtMaster->Schedule(timeoutTimer);
    evtMaster->Run();
    delete timeoutTimer;
}

// Actaul XMLRPC methods
void XMLRPC_ComputePathMethod::execute(xmlrpc_c::paramList const& paramList, xmlrpc_c::value *   const  retvalP) 
{   
    if (msgPort == NULL)
        this->init();

    // parse xmlrpc params
    map<string, xmlrpc_c::value> reqStruct = paramList.getStruct(0);
    string urn = xmlrpc_c::value_string(reqStruct["slice_urn"]);
    string rspec = xmlrpc_c::value_string(reqStruct["request_rspec"]);
    map<string, xmlrpc_c::value> options = xmlrpc_c::value_struct(reqStruct["request_options"]);
    bool hold_path = false;
    u_int32_t start_time = 0, end_time = 0;
    if (options.find("geni-hold-path") != options.end()) {
        hold_path = xmlrpc_c::value_boolean(options["geni-hold-path"]);
    }
    if (options.find("geni-start-time") != options.end()) {
        start_time = xmlrpc_c::value_i8(options["geni-start-time"]);
    }
    if (options.find("geni-end-time") != options.end()) {
        end_time = xmlrpc_c::value_i8(options["geni-end-time"]);
    }

    string queueName="CORE";
    string topicName="XMLRPC_API_REQUEST";
    Message* testMsg = new Message(MSG_REQ, queueName, topicName);
    // TODO: parse RSpec XML and compose API request TLVs
        
    msgPort->PostMessage(testMsg);
    this->fire();

    // poll MessagePort queue:
    if (msgPort->GetMsgInQueue().size() == 0) 
    {
        // TODO: create xmlrpc error
    }
    else {
        // TODO: relate message using contextTag 
        // create reply msg
    }
}


// Server Thread 
void* XMLRPC_APIServer::Run()
{
    try {
        xmlrpc_c::registry myRegistry;
        xmlrpc_c::methodPtr const computePathMethodP(new XMLRPC_ComputePathMethod(mxTCE));
        myRegistry.addMethod("ComputePath", computePathMethodP);
        xmlrpc_c::serverAbyss myAbyssServer(
            xmlrpc_c::serverAbyss::constrOpt()
            .registryP(&myRegistry)
            .portNumber(MxTCE::xmlrpcApiServerPort));
        myAbyssServer.run();
        // xmlrpc_c::serverAbyss.run() never returns
    } catch (exception const& e) {
        LOG("XMLRPC_APIServer failed: " << e.what() << endl);
    }
    return 0;
}

