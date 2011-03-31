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

#include "types.hh"
#include "event.hh"
#include "utils.hh"
#include "exception.hh"
#include "log.hh"
#include "api.hh"
#include "request_encoder.hh"

#ifndef HAVE_DECL_GETOPT
  #define HAVE_DECL_GETOPT 1
#endif
#include <getopt.h>

struct option longopts[] = 
{
    { "config", required_argument, NULL, 'c'},
    { "host",   required_argument, NULL, 'h'},
    { "port",   required_argument, NULL, 'p'},
    { "src_urn",   required_argument, NULL, 'S'},
    { "dst_urn",   required_argument, NULL, 'D'},
    { "bandwidth",   required_argument, NULL, 'B'},
    { "start_time",   required_argument, NULL, 's'},
    { "end_time",   required_argument, NULL, 'e'},
    { "src_vlan",   required_argument, NULL, 'u'},
    { "dst_vlan",   required_argument, NULL, 'v'},
    { 0 }
};

void showUsage()
{
    cout<<"MX-TCE Tester Usage:"<<endl;
    cout<<"\t mxtce_test [-c test config file] [-h host) [-p port] -S src_urn -D dst_urn -B bandwidth [-s starttime] [-e endtime] -u src_vlan -d dst_vlan" <<endl;
    cout<<"\t mxtce_test [-c test config file] [-h host) [-p port] -S src_urn -D dst_urn -B bandwidth [-s starttime] [-e endtime] -u src_vlan -d dst_vlan" <<endl;
    cout<<"\t Example: mxtce_test -S \"urn:ogf:network:domain=testdomain-1.net:node=node-1:port=port-1:link=link-1\" -D \"urn:ogf:network:domain=testdomain-1.net:node=node-2:port=port-2:link=link-1\" -B 100M -s 1305601600 -e 1305801600 -u 1234 -d 2345" <<endl;
}

int main( int argc, char* argv[])
{
    bool is_daemon = false;
    LogOption log_opt = LOG_STDOUT;
	const char* configfile = "/usr/local/etc/mxtce_test.conf";
    char* api_host = (char*)"localhost";
    int api_port = 2089;

	string gri="";
	u_int32_t start_time=0;
	u_int32_t end_time=0;
	u_int32_t bandwidth=0;
	string layer="2";
	string path_setup_model="";
	string path_type="";
	string src_vlan_tag="";
	string dest_vlan_tag="";
	string src_end_point="";
	string dest_end_point="";
	u_int32_t src_ip_port=0;
	u_int32_t dest_ip_port=0;
	string protocol="";
	string dscp="";
	u_int32_t burst_limit=40;
	string lsp_class="";

    while (1)
    {
        int opt;

        opt = getopt_long (argc, argv, "c:h:p:s:e:u:v:B:S:D:", longopts, 0);
        if (opt == EOF)
            break;

        switch (opt) 
        {
        case 'c':
            configfile = optarg;
            break;
        case 'h':
            api_host = optarg;
            break;
        case 'p':
            api_port = atoi(optarg);
            break;
        case 'g':
            gri = optarg;
            break;
        case 'B':
            {
            string bw_str = optarg;
            bw_str = StringToBandwidth(bw_str);
            bandwidth = atoi(bw_str.c_str());
            }
            break;
        case 'S':
            src_end_point = optarg;
            break;
        case 'D':
            dest_end_point = optarg;
            break;
        case 's':
            start_time = atoi(optarg);
            break;
        case 'e':
            end_time = atoi(optarg);
            break;
        case 'u':
            src_vlan_tag = optarg;
            break;
        case 'v':
            dest_vlan_tag = optarg;
            break;
        case 'l':
            layer = optarg;
            break;
        default:
            showUsage();
            break;
        }
    }

    Log::Init(log_opt, "");
    Log::SetDebug(true);

    Apimsg_user_constraint* user_cons = new Apimsg_user_constraint();
    user_cons->setGri(gri);
    user_cons->setStarttime(start_time);
    user_cons->setEndtime(end_time);
    user_cons->setBandwidth(bandwidth);
    user_cons->setPathsetupmodel(path_setup_model);
    user_cons->setPathtype(path_type);
    user_cons->setLayer(layer);
    user_cons->setSrcendpoint(src_end_point);
    user_cons->setDestendpoint(dest_end_point);
    if(layer=="2")
    {
        user_cons->setSrcvlantag(src_vlan_tag);
        user_cons->setDestvlantag(dest_vlan_tag);
    }
    else if(layer=="3")
    {
        user_cons->setSrcipport(src_ip_port);
        user_cons->setDestipport(dest_ip_port);
        user_cons->setProtocol(protocol);
        user_cons->setDscp(dscp);
        user_cons->setBurstlimit(burst_limit);
        user_cons->setLspclass(lsp_class);
    }

    Apireqmsg_encoder* encoder = new  Apireqmsg_encoder();
    api_msg* msg = encoder->test_encode_msg(user_cons);
    assert(msg != NULL);
    APIClient* client = new APIClient(api_host, api_port);
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



