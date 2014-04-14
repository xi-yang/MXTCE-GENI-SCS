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
#include "rspec.hh"
#include "workflow.hh"
#include <map>
#include <algorithm>

Lock XMLRPC_APIServer::xmlrpcApiLock; // lock to assure only one API call is served at a time

// Base class 
// TODO: move XMLRPC_BaseMethod::msgPort to thread level (or make class static) if more than one method
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
            LOG("XMLRPC_APIServer::Run caught Exception: " << e.what() << endl);
        }
    }
}

void XMLRPC_BaseMethod::fire()
{
    // TODO: use callback to stop eventmaster (need to modify MessagePort to hook up extra callback)
    XMLRPC_TimeoutOrCallback* timeoutOrCallback = new XMLRPC_TimeoutOrCallback(evtMaster);
    assert(evtMaster);
    msgPort->GetMsgInQueue().clear();
    this->msgPort->SetMessageCallback(timeoutOrCallback);
    evtMaster->Schedule(timeoutOrCallback);
    evtMaster->Run();
    evtMaster->Remove(timeoutOrCallback);
    delete timeoutOrCallback;
}

// Actual XMLRPC methods
void XMLRPC_ComputePathMethod::execute(xmlrpc_c::paramList const& paramList, xmlrpc_c::value* const retvalP) 
{
    XMLRPC_APIServer::xmlrpcApiLock.DoLock();

    if (msgPort == NULL)
        this->init();

    // parse xmlrpc params
    map<string, xmlrpc_c::value> reqStruct = paramList.getStruct(0);
    string urn = xmlrpc_c::value_string(reqStruct["slice_urn"]);
    string rspec = xmlrpc_c::value_string(reqStruct["request_rspec"]);
    LOGF("XMLRPC_ComputePath (slice_urn='%s') begins with request_rspec: \n%s\n", urn.c_str(), rspec.c_str());
    map<string, xmlrpc_c::value> options = xmlrpc_c::value_struct(reqStruct["request_options"]);
    bool hold_path = false;
    bool workflow_paths_merged = false;
    map<string, xmlrpc_c::value> routing_profile;
    u_int32_t start_time = 0, end_time = 0;
    if (options.find("geni_hold_path") != options.end()) {
        hold_path = xmlrpc_c::value_boolean(options["geni_hold_path"]);
    }
    if (options.find("geni_start_time") != options.end()) {
        start_time = xmlrpc_c::value_i8(options["geni_start_time"]);
    }
    if (options.find("geni_end_time") != options.end()) {
        end_time = xmlrpc_c::value_i8(options["geni_end_time"]);
    }
    if (options.find("geni_routing_profile") != options.end()) {
        routing_profile = xmlrpc_c::value_struct(options["geni_routing_profile"]);
    }
    if (options.find("geni_workflow_paths_merged") != options.end()) {
        workflow_paths_merged = xmlrpc_c::value_boolean(options["geni_workflow_paths_merged"]);
    }
    
    GeniRequestRSpec reqRspec(rspec);
    string contextTag = "";
    Message* reqMsg = NULL;
    try {
        reqMsg = reqRspec.CreateApiRequestMessage(routing_profile);
    } catch (TEDBException ex) {
            ReturnGeniError(retvalP, GENI_PCS_ERRCODE_MAILFORMED_REQUEST, ex.GetMessage().c_str());
            goto _final;        
    } catch (exception exx) {
            ReturnGeniError(retvalP, GENI_PCS_ERRCODE_UNKNOWN, exx.what());
            goto _final;        
    }
    contextTag = reqMsg->GetContextTag();
    // TODO: append option TLVs (hold_path, exclusion_list ...)
    msgPort->PostMessage(reqMsg);
    this->fire();

    // poll MessagePort queue:
    if (msgPort->GetMsgInQueue().size() == 0) 
    {
        ReturnGeniError(retvalP, GENI_PCS_ERRCODE_TIMEOUT, "Timeout: no response received from computing core!");
        // TODO: define error codes
        goto _final;
    }
    else {
        list<Message*>::iterator itm = msgPort->GetMsgInQueue().begin();
        for (; itm != msgPort->GetMsgInQueue().end(); itm++) 
        {
            Message* replyMsg = *itm;
            if (replyMsg->GetContextTag().compare(contextTag) == 0)
            {
                GeniManifestRSpec replyRspec(&reqRspec);
                try {
                    replyRspec.ParseApiReplyMessage(replyMsg);
                } catch (TEDBException ex) {
                    msgPort->GetMsgInQueue().remove(replyMsg);
                    delete replyMsg;
                    ReturnGeniError(retvalP, GENI_PCS_ERRCODE_INCOMPLETE_REPLY, ex.GetMessage().c_str());
                    goto _final;        
                }
                msgPort->GetMsgInQueue().remove(replyMsg);
                delete replyMsg;
                string service_rspec = replyRspec.GetRspecXmlString();
                map<string, xmlrpc_c::value> retMap;
                map<string, xmlrpc_c::value> codeMap;
                map<string, xmlrpc_c::value> valueMap;
                codeMap["geni_code"] = xmlrpc_c::value_int(GENI_PCS_ERRCODE_NO_ERROR);
                retMap["code"] = xmlrpc_c::value_struct(codeMap);
                valueMap["service_rspec"] = xmlrpc_c::value_string(service_rspec);
                // workflow data
                if (!replyRspec.GetWorkflowDataMap().empty())
                {
                    map<string, xmlrpc_c::value> retWfdMap;
                    map<string, WorkflowData*>::iterator itW = replyRspec.GetWorkflowDataMap().begin();
                    list<WorkflowData*> sortedArray;
                    for (; itW != replyRspec.GetWorkflowDataMap().end(); itW++)
                    {
                        // xml form of workflow data for individual path
                        retWfdMap[(*itW).first] = *((WorkflowData*)(*itW).second)->GetXmlRpcData();
                        // add to sorted array of the workflow data in ascending order of path length
                        list<WorkflowData*>::iterator itWV = sortedArray.begin();
                        for (; itWV != sortedArray.end(); itWV++) 
                        {
                            if ((*itWV)->GetDependencies().size() < ((WorkflowData*)(*itW).second)->GetDependencies().size())
                            {
                                itWV = sortedArray.insert(itWV, (*itW).second);
                                break;
                            }
                        }                        
                        if (itWV == sortedArray.end())
                        {
                            sortedArray.push_back((*itW).second);
                        }
                    }
                    // merge multi-path workflow data in descending order of path length
                    if (workflow_paths_merged)
                    {
                        list<WorkflowData*>::reverse_iterator ritW = sortedArray.rbegin();
                        WorkflowData* combinedWorkflowData = *ritW;
                        ritW++;
                        for (; ritW != sortedArray.rend(); ritW++)
                        {
                            combinedWorkflowData->MergeDependencies((*ritW)->GetDependencies());
                        }
                        combinedWorkflowData->GenerateXmlRpcData();
                        retWfdMap["##all_paths_merged##"] = *combinedWorkflowData->GetXmlRpcData();
                    }
                    valueMap["workflow_data"] = xmlrpc_c::value_struct(retWfdMap);
                }
                retMap["value"] = xmlrpc_c::value_struct(valueMap);
                *retvalP = xmlrpc_c::value_struct(retMap);
                goto _final;        
            }
        }
    }

_final:

    LOGF("XMLRPC_ComputePath(slice_urn='%s') ends!\n", urn.c_str());
    XMLRPC_APIServer::xmlrpcApiLock.Unlock();
}

void XMLRPC_ComputePathMethod::ReturnGeniError(xmlrpc_c::value* const retvalP, int errCode, const char* errMsg)
{
    map<string, xmlrpc_c::value> retMap;
    map<string, xmlrpc_c::value> codeMap;
    codeMap["geni_code"] = xmlrpc_c::value_int(errCode);
    retMap["code"] = xmlrpc_c::value_struct(codeMap);
    retMap["output"] = xmlrpc_c::value_string(errMsg);
    *retvalP = xmlrpc_c::value_struct(retMap);
}


void XMLRPC_GetVersionMethod::execute(xmlrpc_c::paramList const& paramList, xmlrpc_c::value* const retvalP) 
{
    XMLRPC_APIServer::xmlrpcApiLock.DoLock();
    map<string, xmlrpc_c::value> retMap;
    retMap["geni_api"] = xmlrpc_c::value_int(2);
    map<string, xmlrpc_c::value> codeMap;
    codeMap["geni_code"] = xmlrpc_c::value_int(0);
    retMap["code"] = xmlrpc_c::value_struct(codeMap);
    map<string, xmlrpc_c::value> valueMap;
    // compose fixed 'value' struct
    string verStr = getVersionString();
    replace( verStr.begin(), verStr.end(), '$', ' ');
    valueMap["code_tag"] = xmlrpc_c::value_string(verStr.c_str());
    valueMap["interface"] = xmlrpc_c::value_string("scs");
    char hostname[32];
    gethostname(hostname, sizeof(hostname));
    char urlCstr[128];
    snprintf(urlCstr, 127, "http://%s:%d/%s", hostname, MxTCE::xmlrpcApiServerPort, MxTCE::xmlrpcApiServerPath.c_str());
    valueMap["url"] = xmlrpc_c::value_string(urlCstr);
    map<string, xmlrpc_c::value> apiVersionsMap;
    apiVersionsMap["2"] = xmlrpc_c::value_string(urlCstr);
    valueMap["geni_api_versions"] = xmlrpc_c::value_struct(apiVersionsMap);   
    vector<xmlrpc_c::value> extArray;
    extArray.push_back(xmlrpc_c::value_string("http://hpn.east.isi.edu/rspec/ext/stitch/0.1/"));
    extArray.push_back(xmlrpc_c::value_string("http://hpn.east.isi.edu/rspec/ext/stitch/2/"));
    map<string, xmlrpc_c::value> requestVersionsMap;
    requestVersionsMap["version"] = xmlrpc_c::value_string("3");
    requestVersionsMap["type"] = xmlrpc_c::value_string("GENI");
    requestVersionsMap["schema"] = xmlrpc_c::value_string("http://www.geni.net/resources/rspec/3/request.xsd");
    requestVersionsMap["namespace"] = xmlrpc_c::value_string("http://www.geni.net/resources/rspec/3");
    requestVersionsMap["extensions"] = xmlrpc_c::value_array(extArray);
    valueMap["geni_request_rspec_versions"] = xmlrpc_c::value_struct(requestVersionsMap);
    // end composing 'value' struct
    retMap["value"] = xmlrpc_c::value_struct(valueMap);
    *retvalP = xmlrpc_c::value_struct(retMap);
    XMLRPC_APIServer::xmlrpcApiLock.Unlock();
}


void XMLRPC_ListAggregatesMethod::execute(xmlrpc_c::paramList const& paramList, xmlrpc_c::value* const retvalP) 
{
    XMLRPC_APIServer::xmlrpcApiLock.DoLock();
    map<string, xmlrpc_c::value> retMap;
    retMap["geni_api"] = xmlrpc_c::value_int(2);
    map<string, xmlrpc_c::value> codeMap;
    codeMap["geni_code"] = xmlrpc_c::value_int(0);
    retMap["code"] = xmlrpc_c::value_struct(codeMap);
    map<string, xmlrpc_c::value> valueMap;
    // compose fixed 'value' struct
    string verStr = getVersionString();
    replace( verStr.begin(), verStr.end(), '$', ' ');
    valueMap["code_tag"] = xmlrpc_c::value_string(verStr.c_str());
    valueMap["interface"] = xmlrpc_c::value_string("scs");
    
    map<string, xmlrpc_c::value> aggregateListMap;
    map<string, string>::iterator itau = GeniAdRSpec::aggregateUrnMap.begin();
    for (; itau != GeniAdRSpec::aggregateUrnMap.end(); itau++) {
        map<string, xmlrpc_c::value> aggregateMap;
        aggregateMap["urn"] = xmlrpc_c::value_string((*itau).second.c_str());
        if (GeniAdRSpec::aggregateUrlMap.find((*itau).first) != GeniAdRSpec::aggregateUrlMap.end()) {
            aggregateMap["url"] = xmlrpc_c::value_string(GeniAdRSpec::aggregateUrlMap[(*itau).first].c_str());
        }
        aggregateListMap[(*itau).first] = xmlrpc_c::value_struct(aggregateMap);
    }
    valueMap["geni_aggregate_list"] = xmlrpc_c::value_struct(aggregateListMap);   
    retMap["value"] = xmlrpc_c::value_struct(valueMap);
    *retvalP = xmlrpc_c::value_struct(retMap);
    XMLRPC_APIServer::xmlrpcApiLock.Unlock();
}

// Server Thread 
void* XMLRPC_APIServer::Run()
{
    try {
        xmlrpc_c::registry myRegistry;
        xmlrpc_c::methodPtr const computePathMethodP(new XMLRPC_ComputePathMethod(mxTCE));
        xmlrpc_c::methodPtr const getVersionMethodP(new XMLRPC_GetVersionMethod(mxTCE));
        xmlrpc_c::methodPtr const listAggregatesMethodP(new XMLRPC_ListAggregatesMethod(mxTCE));
        myRegistry.addMethod("ComputePath", computePathMethodP);
        myRegistry.addMethod("GetVersion", getVersionMethodP);
        myRegistry.addMethod("ListAggregates", listAggregatesMethodP);
        xmlrpc_c::serverAbyss myAbyssServer(
            xmlrpc_c::serverAbyss::constrOpt()
            .registryP(&myRegistry)
            .portNumber(MxTCE::xmlrpcApiServerPort)
            .uriPath(MxTCE::xmlrpcApiServerPath));
        myAbyssServer.run();
        // xmlrpc_c::serverAbyss.run() never returns
    } catch (exception const& e) {
        LOG("XMLRPC_APIServer failed: " << e.what() << endl);
    }
    return 0;
}

