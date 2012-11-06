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

#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/registry.hpp>
#include <xmlrpc-c/server_abyss.hpp>
 

using namespace std;

class MxTCE;
class XMLRPC_BaseMethod: public xmlrpc_c::method {
protected:
    MxTCE *mxTCE;
    MessagePort* msgPort;
        
public:
    XMLRPC_BaseMethod(MxTCE* tce):mxTCE(tce) { 
        this->_signature = "Base";
        this->_help = "base method";
        msgPort = NULL;
    }
    void execute(xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP) {
    }
    void begin();
    void end();
};

class XMLRPC_ComputePathMethod: public XMLRPC_BaseMethod {
public:
    XMLRPC_ComputePathMethod(MxTCE* tce):XMLRPC_BaseMethod(tce) {
        this->_signature = "ComputePath";
        this->_help = "compute path method";
    }
    void execute(xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);
};


class XMLRPC_APIServer: public Thread
{
protected:
    MxTCE* mxTCE;

public:
    XMLRPC_APIServer(MxTCE* tce):mxTCE(tce) { }
    virtual ~XMLRPC_APIServer() { }
    virtual void* Run();
};

#define XMLRPC_APIServerThread XMLRPC_APIServer
#define MAX_MSG_PORT_POLL_TIME 10 // seconds

#endif

