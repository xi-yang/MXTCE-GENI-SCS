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


#ifndef __MXTCE_HH__
#define __MXTCE_HH__

#include "message.hh"
#include "apiserver.hh"

class MxTCE
{
private:
    MessagePortLoopback* loopbackPort;
    EventMaster* eventMaster;
    MessageRouter* messageRouter;
    APIServerThread* apiServerThread;
    
public:
    static int apiServerPort;
    static string loopbackPortName;
    static string apiServerPortName;
    static string tedbManPortName;
    static string resvManPortName;
    static string policyManPortName;

public:
	MxTCE( const string& configFile);
	~MxTCE();
    MessagePortLoopback* GetLoopbackPort() { return loopbackPort; }
    MessageRouter* GetMessageRouter() { return messageRouter; }
    void Start();
    void CheckMessage();
};


class MxTCEMessageHandler: public Event
{
private:
    MxTCEMessageHandler();
    MxTCE* mxTCE;

public:
    MxTCEMessageHandler(MxTCE* tce): mxTCE(tce) { }
    ~MxTCEMessageHandler() { }
    virtual void Run();
};

    
#endif
