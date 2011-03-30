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


#ifndef __RESV_APISERVER_HH__
#define __RESV_APISERVER_HH__

#include "types.hh"
#include "event.hh"
#include "thread.hh"
#include "api.hh"
#include "resource.hh"

using namespace std;

class ResvAPIServer: public APIServer
{
protected:
    ResvManThread* resvManThread;
public:
    ResvAPIServer(int port , ResvManThread* thread): APIServer(port, NULL), resvManThread(thread) {
        assert (thread);
    }
    virtual ~ResvAPIServer() { } 
    virtual int HandleAPIMessage (APIReader* apiReader, APIWriter* apiWriter, api_msg* apiMsg);
};

typedef struct {
    u_int16_t type;    // MSG_TLV_RESV_INFO  : ntohs for external to internal msg conversion
    u_int16_t length; //ntohs for external to internal msg conversion
    char gri[64]; //c_str
    u_int32_t start_time;
    u_int32_t end_time;
    u_int32_t bandwidth;
    char status[16]; //c_str
} TLV_ResvInfo;

typedef struct {
    u_int16_t type;    // MSG_TLV_PATH_ELEM  : ntohs for external to internal msg conversion
    u_int16_t length; //ntohs for external to internal msg conversion
    char urn[128]; //c_str
    u_int8_t switching_type;
    u_int8_t encoding_type;
    u_int16_t vlan; //only tak reservation in 'ACIVE' or 'PENDING' status that means fixed single VLAN ID
} TLV_PathElem;

#endif

