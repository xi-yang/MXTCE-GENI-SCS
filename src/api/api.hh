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


#ifndef __API_HH__
#define __API_HH__

#include "types.hh"
#include "event.hh"

using namespace std;

#define API_MAX_MSG_SIZE 102400

enum api_msg_type
{
    API_MSG_REQUEST = 0x0001,
    API_MSG_REPLY = 0x0002,
    API_MSG_ACK = 0x0003,
    API_MSG_RESV_PUSH = 0x0011,
};   

enum api_msg_option
{
    API_OPT_GROUP = 0x00000001, // indicaate the message belong to a group
    API_OPT_GROUP_LAST = 0x00000002, // indicate the message is the last in a group
};

struct api_msg_header
{
    u_int16_t type;
    u_int16_t length;
    u_int32_t ucid; //  unique client id
    u_int32_t seqnum; // sequence number, specified by requestor
    u_int32_t chksum;   // checksum for the above three 32-b words
    union
    {
        u_int32_t msgtag[2];
        struct 
        {
            u_int32_t options;
            u_int32_t tag;
        };
    };
};

struct api_msg
{
    struct api_msg_header header;
    char * body;
};

struct api_tlv_header
{  
    u_int16_t type;
    u_int16_t length;
};

#define MSG_CHKSUM(X) (((u_int32_t*)&X)[0] + ((u_int32_t*)&X)[1] + ((u_int32_t*)&X)[2])

api_msg * api_msg_new (u_int16_t msgtype, u_int16_t msglen, void *msgbody, u_int32_t ucid, u_int32_t seqnum,  u_int32_t tag = 0);
api_msg* api_msg_new (u_char msgtype, u_char action, u_int16_t msglen, void *msgbody, u_int32_t ucid, u_int32_t seqnum,  u_int32_t tag = 0);
struct api_msg * api_msg_append_tlv (struct api_msg * msg, struct api_tlv_header* tlv);
void api_msg_delete (struct api_msg* msg);
void api_msg_dump (struct api_msg* msg);


///////////// API Server side classes ////////////

class APIReader;
class APIWriter;
class APIServer: public Reader
{
protected:
    u_int32_t port;
    EventMaster* eventMaster;
    APIServer();

public:
    APIServer(u_int32_t port_val, EventMaster* em):Reader(0), port(port_val), eventMaster(em) { }
    virtual ~APIServer() {}
    EventMaster* GetEventMaster() { return eventMaster; }
    void SetEventMaster(EventMaster* evm) { eventMaster = evm; }
    void Start();
    void Exit();
    virtual void Run();
    virtual APIReader* CreateAPIReader(int sock);
    virtual APIWriter* CreateAPIWriter(int sock);
    virtual int HandleAPIMessage (APIReader*, APIWriter*, api_msg *) = 0; // pure virtual
};

class APIWriter;
class APIReader: public Reader
{
protected:
    APIServer *server;
    APIWriter* api_writer;
public:
    APIReader(int fd, APIServer *server_ptr):Reader(fd), server(server_ptr) 
        { }
    virtual ~APIReader() {}
    virtual void Run ();
    APIWriter* GetWriter() { return api_writer; }    
    void SetWriter(APIWriter* writer) { api_writer = writer; }
    api_msg * ReadMessage();
};


#define URGENT_POST true

class APIWriter: public Writer
{
protected:
    APIServer *server;
    list<api_msg *> msgQueue;
    APIReader* api_reader;
public:
    APIWriter(int fd, APIServer *server_ptr): Writer(fd), server(server_ptr) { }
    virtual ~APIWriter() {}
    virtual void Run ();
    APIReader* GetReader() { return api_reader; }    
    void SetReader(APIReader* reader) { api_reader = reader; }
    void WriteMessage(api_msg *msg);
    void PostMessage(api_msg *msg, bool urgent = (!URGENT_POST));
};



///////////// API Clinet side classe ////////////

class APIClient: public APIReader
{
protected:
    string _host;
    int _port;

public:
    APIClient();
    APIClient(char* host, int port);    
    virtual ~APIClient();
    void SetHostPort(char *host, int port);
    void Connect(char *host, int port);
    void Connect();
    bool IsAlive();
    void SendMessage(api_msg *msg);
};


#endif

