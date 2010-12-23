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

#include "types.hh"
#include "event.hh"
#include "utils.hh"
#include "exception.hh"
#include "log.hh"
#include "api.hh"

#ifndef HAVE_DECL_GETOPT
  #define HAVE_DECL_GETOPT 1
#endif
#include <getopt.h>

struct option longopts[] = 
{
    { "config", required_argument, NULL, 'c'},
    { "host",   required_argument, NULL, 'h'},
    { "port",   required_argument, NULL, 'p'},
    { 0 }
};

void showUsage()
{
    cout<<"MX-TCE Tester Usage:"<<endl;
    cout<<"\t mxtce_test [-c test config file] [-h] (host) [-p port]" <<endl;
}

int main( int argc, char* argv[])
{
    bool is_daemon = false;
    LogOption log_opt = LOG_STDOUT;
	const char* configfile = "/usr/local/etc/mxtce_test.conf";
    char* api_host = (char*)"localhost";
    int api_port = 2089;

    while (1)
    {
        int opt;

        opt = getopt_long (argc, argv, "c:p:h", longopts, 0);
        if (opt == EOF)
            break;

        switch (opt) 
        {
        case 'c':
            configfile = optarg;
            break;
            break;
        case 'h':
            api_host = optarg;
            break;
        case 'p':
            api_port = atoi(optarg);
            break;
        default:
            showUsage();
            break;
        }
    }

    Log::Init(log_opt, "");
    Log::SetDebug(true);

    APIClient* client = new APIClient(api_host, api_port);
    api_tlv_header* tlv = (api_tlv_header*)new char[8];
    *(int*)(tlv+1) = 1234;
    api_msg* msg = api_msg_new(API_MSG_REQUEST,8,tlv, 0x0001,0x0001,0);
    try {
        client->Connect();
        client->SendMessage(msg);
        msg = client->ReadMessage();
    } catch (TCEException e) {
        LOG("MxTCE Test API client caught APIException with errMsg: " << e.GetMessage() <<endl);
        return -1;
    }
    api_msg_dump(msg);
    return 0;
}



