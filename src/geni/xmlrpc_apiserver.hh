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


#ifndef __XMLRPC_APISERVER_HH__
#define __XMLRPC_APISERVER_HH__

#include "types.hh"
#include "event.hh"
#include "thread.hh"
#include "resource.hh"
#include "api.hh"

#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/registry.hpp>
#include <xmlrpc-c/server_abyss.hpp>

#include <set>

using namespace std;

class MxTCE;
class XMLRPC_BaseMethod: public xmlrpc_c::method {
protected:
    MxTCE *mxTCE;
    MessagePort* msgPort;
    EventMaster* evtMaster;
        
public:
    XMLRPC_BaseMethod(MxTCE* tce):mxTCE(tce) { 
        this->_help = "base method";
        msgPort = NULL;
        evtMaster = NULL;
    }
    void execute(xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP) {
    }
    void init();
    void fire();
};

class XMLRPC_TimeoutOrCallback: public Timer, public MessagePortCallback {
private:
    EventMaster* evtMaster;
    MessagePort* msgPort;
    XMLRPC_TimeoutOrCallback() { }
    static set<string> discardedContextTags;
    
public:
    XMLRPC_TimeoutOrCallback(EventMaster* em, MessagePort* mp): Timer((int)APIServer::maxApiTimeOutSecs, (int)0), evtMaster(em), msgPort(mp){ }
    virtual void Run();
    virtual void hookRunCallback();
    static set<string>& getDiscardedContextTags () {
        return discardedContextTags;
    }
};

class XMLRPC_ComputePathMethod: public XMLRPC_BaseMethod {    
public:
    XMLRPC_ComputePathMethod(MxTCE* tce):XMLRPC_BaseMethod(tce) {
        this->_signature = "?";
        this->_help = "compute_path method";
    }
    void execute(xmlrpc_c::paramList const& paramList, xmlrpc_c::value * const  retvalP);
    void ReturnGeniError(xmlrpc_c::value* const retvalP, int errCode, const char* errMsg);
};

class XMLRPC_GetVersionMethod: public XMLRPC_BaseMethod {    
public:
    XMLRPC_GetVersionMethod(MxTCE* tce):XMLRPC_BaseMethod(tce) {
        this->_signature = "?";
        this->_help = "get_version method";
    }
    void execute(xmlrpc_c::paramList const& paramList, xmlrpc_c::value * const  retvalP);
};

class XMLRPC_ListAggregatesMethod: public XMLRPC_BaseMethod {    
public:
    XMLRPC_ListAggregatesMethod(MxTCE* tce):XMLRPC_BaseMethod(tce) {
        this->_signature = "?";
        this->_help = "list_aggregates method";
    }
    void execute(xmlrpc_c::paramList const& paramList, xmlrpc_c::value * const  retvalP);
};

#define XMLRPC_APIServerThread XMLRPC_APIServer

class XMLRPC_APIServer: public Thread
{
protected:
    MxTCE* mxTCE;

public:
    static Lock xmlrpcApiLock;

    XMLRPC_APIServer(MxTCE* tce):mxTCE(tce) { }
    virtual ~XMLRPC_APIServer() { }
    virtual void* Run();
};


#endif

