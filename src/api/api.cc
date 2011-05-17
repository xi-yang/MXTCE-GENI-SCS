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
#include "exception.hh"
#include "utils.hh"
#include "log.hh"
#include "api.hh"



// TODO: Exception handling

void APIServer::Start()
{
    int sock;
    int ret = -1;
    struct sockaddr_in sa_in;
    memset (&sa_in, 0, sizeof (struct sockaddr_in));

    char buf[128];
    //Get a stream socket
    sock = socket (AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        strcpy(buf, "APIServer::Start() can't make socket sockunion_stream_socket");
        LOG(buf<<endl);
        throw APIException(buf);
    }
  
    // reuse address and port on the server
    int on = 1;
    ret = setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (void *) &on, sizeof (on));
    if (ret < 0)
    {
        snprintf(buf, 128, "APIServer::Start() can't set sockopt SO_REUSEADDR to socket %d", sock);
        LOG(buf<<endl);
        throw APIException(buf);
    }
#ifdef SO_REUSEPORT
    ret = setsockopt (sock, SOL_SOCKET, SO_REUSEPORT, (void *) &on, sizeof (on));
    if (ret < 0)
    {
        snprintf(buf, 128, "APIServer::Start() can't set sockopt SO_REUSEPORT to socket %d", sock);
        throw APIException(buf);

    }
#endif

    // bind socket
    assert (port > 0);

    int size = sizeof(struct sockaddr_in);
    sa_in.sin_family = AF_INET;
    sa_in.sin_port = htons(port);
    u_int32_t addr_u32 = htonl(INADDR_ANY);
    memcpy(&(sa_in.sin_addr), &addr_u32, sizeof(struct in_addr));
    ret = bind (sock, (sockaddr *)&sa_in, size);
    if (ret < 0)
    {
        snprintf(buf, 128, "APIServer::Start() can't bind socket : %s", strerror (errno));
        LOG(buf<<endl);
        close (sock);
        throw APIException(buf);
    }

    // Listen under queue length 10
    ret = listen (sock, 9);
    if (ret < 0)
    {
        snprintf(buf, 128, "APIServer::Start()::listen: %s", strerror (errno));
        LOG(buf <<endl);
        close (sock);
        throw APIException(buf);
    }

    //Sechedule itself
    this->fd = sock;
    this->SetRepeats(FOREVER);
    eventMaster->Schedule(this);
}

void APIServer::Exit()
{
    if (!this->Obsolete())
        eventMaster->Remove(this);
}

APIReader* APIServer::CreateAPIReader(int sock)
{
    return (new APIReader(sock, this));
}

APIWriter* APIServer::CreateAPIWriter(int sock)
{
    return (new APIWriter(sock, this));
}

void APIServer::Run()
{
    struct sockaddr_in sa_in;
    sa_in.sin_family = AF_INET;
    sa_in.sin_port = htons(port);
    u_int32_t addr_u32 = htonl(INADDR_ANY);
    memcpy(&(sa_in.sin_addr), &addr_u32, sizeof(struct in_addr));
    socklen_t len = sizeof(struct sockaddr_in);
    int new_sock = accept (fd, (struct sockaddr *)&sa_in, &len);
    if (new_sock < 0)
    {
        char buf[128];
        snprintf(buf, 128, "APIServer::Run() Cannot accept socket on %d", fd);
        LOG(buf<<endl);
        throw APIException(buf);
    }

    APIWriter* api_writer = CreateAPIWriter(new_sock);
    api_writer->SetAutoDelete(false);
    api_writer->SetRepeats(0);

    APIReader* api_reader = CreateAPIReader(new_sock);
    api_reader->SetAutoDelete(true);
    api_reader->SetRepeats(FOREVER);
    api_reader->SetWriter(api_writer);
    api_writer->SetReader(api_reader);
    
    //Schedule read and write events for each accepted connection
    //eventMaster->Schedule(api_writer);
    eventMaster->Schedule(api_reader);

    LOG_DEBUG("APIServer::Run() Accepted an API connection on socket(" <<new_sock <<")" << endl);
}


/////////////////////////////////////////////////////////////////

void APIReader::Run()
{
    Reader::Run();

    //read an API message
    if (fd < 0)
        return;

    api_msg * msg = NULL;
    try {
        msg = ReadMessage();
    } catch (APIException& e) {
        //something is wrong with socket read
        assert(api_writer);
        api_writer->Close();
        Close();
        //do not escalate
        //throw;
        return;
    }

    //Message handling by using APIServer callback
    server->HandleAPIMessage(this, this->api_writer, msg); 
}

api_msg * APIReader::ReadMessage ()
{
    api_msg* msg;
    api_msg_header header;
    char buf[API_MAX_MSG_SIZE];
    int bodylen;
    int rlen;

    /* Read message header */
    rlen = readn (fd, (char *) &header, sizeof (api_msg_header));



    if (rlen < 0)
    {
        snprintf(buf, 128, "APIReader::ReadMessage () failed to read from %d", fd);
        LOG(buf<<endl);
        throw APIException(buf);
    }
    else if (rlen == 0)
    {
        close(fd);
        snprintf(buf, 128, "APIReader::ReadMessage () connection closed for APIReader(%d)", fd);
        LOG(buf<<endl);
        throw APIException(buf);
    }
    else if (rlen != sizeof (struct api_msg_header))
    {
        snprintf(buf, 128, "APIReader(%d) cannot read the message header", fd);
        LOG(buf << endl);
        throw APIException(buf);
    }

    if (MSG_CHKSUM(header) != header.chksum)
    {
    	snprintf(buf, 128, "APIReader(%d) packet corrupt (ucid=0x%x, seqno=0x%x).\n", fd, ntohl(header.ucid), ntohl(header.seqnum));
        LOG(buf << endl);
        throw APIException(buf);

    }

    // Determine body length. 
    bodylen = ntohs (header.length);
    if (bodylen > API_MAX_MSG_SIZE)
    {
        snprintf(buf, 128, "APIReader(%d) cannot read oversized packet", fd);
        LOG(buf <<endl);
        throw APIException(buf);
    }

    if (bodylen > 0)
    {
        // Read message body
        rlen = readn (fd, buf, bodylen);
        if (rlen < 0)
        {
            snprintf(buf, 128, "APIReader failed to read from %d", fd);
            LOG(buf << endl);
            throw APIException(buf);
        }
        else if (rlen == 0)
        {
            snprintf(buf, 128, "Connection closed for APIReader(%d)", fd);
            LOG(buf << endl);
            throw APIException(buf);
        }
        else if (rlen != bodylen)
        {
            snprintf(buf, 128, "APIReader(%d) cannot read the message body", fd);
            LOG(buf <<endl);
            throw APIException(buf);
        }
    }

    // Allocate new message
    msg = api_msg_new (ntohs(header.type), ntohs(header.length), buf, ntohl(header.ucid), ntohl (header.seqnum));
    msg->header.msgtag[0] = header.msgtag[0];
    msg->header.msgtag[1] = header.msgtag[1];
    return msg;
}


/////////////////////////////////////////////////////////////////

void APIWriter::Run()
{
    Writer::Run();

    //write an API message
    if (fd < 0)
        return;

    if (msgQueue.empty())
        return;

    api_msg * msg = msgQueue.front();
    assert (msg);
    msgQueue.pop_front();
    try {
        WriteMessage(msg); //msg is deleted in WriteMessage
    } catch (APIException& e) {
        //something is wrong with socket write
        assert(api_reader);
        api_reader->Close();
        Close();
        //do not escalate
        //throw;
        return;
    }
    if (msgQueue.size() > 0)
    {
        this->SetObsolete(false);
        this->SetRepeats(1);
    }
}

void APIWriter::WriteMessage (api_msg *msg)
{
    char buf[API_MAX_MSG_SIZE];

    assert (msg);

    // Length of message including header
    int len = sizeof (api_msg_header) + ntohs (msg->header.length);

    // Make contiguous memory buffer for message
    memcpy (buf, &msg->header, sizeof (api_msg_header));
    if (msg->body && ntohs(msg->header.length) > 0)
        memcpy (buf + sizeof (api_msg_header), msg->body, ntohs (msg->header.length));

    if (MSG_CHKSUM(msg->header) != msg->header.chksum)
    {
        snprintf(buf, 128, "APIWriter(%d) packet corrupt", fd);
        LOG(buf<<endl);
        api_msg_delete (msg);
        throw APIException(buf);
    }

    api_msg_delete (msg);

    int wlen = writen(fd, buf, len);
    if (wlen < 0)
    {
        snprintf(buf, 128, "APIWriter failed to write %d", fd);
        LOG(buf << endl);
        throw APIException(buf);
    }
    else if (wlen == 0)
    {
        close(fd);
        snprintf(buf, 128, "Connection closed for APIWriter(%d)", fd);
        LOG(buf << endl);
        throw APIException(buf);
    }
    else if (wlen != len)
    {
        snprintf(buf, 128, "APIWriter(%d) cannot write the message", fd);
        LOG(buf <<endl);
        throw APIException(buf);
    }
}

void APIWriter::PostMessage (api_msg *msg, bool urgent)
{
    assert (msg);

    //cout<<"test8"<<endl;
    //cout<<"msgqueue"<<this->msgQueue.size()<<endl;

    if (urgent == URGENT_POST)
        this->msgQueue.push_front(msg);
    else{
        this->msgQueue.push_back(msg);
        cout<<"test tmp"<<endl;
    }
    //cout<<"test9"<<endl;

    this->SetRepeats(0);

    //cout<<"test10"<<endl;

    this->SetObsolete(false);
    if (msgQueue.size() == 1)
        server->GetEventMaster()->Schedule(this);
}



////////////////////////////// API Client ///////////////////////////
// for test suite

APIClient::APIClient():APIReader(-1, NULL), _port(0)
{
    api_writer = new APIWriter(-1, NULL);
    api_writer->SetReader(this);
}

APIClient::APIClient(char * host, int port):APIReader(-1, NULL)
{
    _host = host; _port = port;
    api_writer = new APIWriter(-1, NULL);
    api_writer->SetReader(this);
}

APIClient::~APIClient()
{
    if (fd > 0)
        Close();

    if (api_writer);
    {
	api_writer->Close();
        delete api_writer;
	api_writer = NULL;
    }
}

void APIClient::SetHostPort(char *host, int port)
{
    _host = host;
    _port = port;
}

void APIClient::Connect(char *host, int port)
{
    struct sockaddr_in addr;
    struct hostent *hp;
    int ret;
    int size;
    int on = 1;

    _host = host;
    _port = port;
  
    assert (strlen(host) > 0 || port > 0);

    char buf[128];
    hp = gethostbyname (host);
    if (!hp)
    {
        snprintf(buf, 128,  "APIClient::Connect: no such host %s", host);
        throw APIException(buf);
    }
  
    fd = socket (AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        snprintf(buf, 128,  "APIClient::Connect: socket(): %s", strerror (errno));
        throw APIException(buf);
    }
                                                                                 
    /* Reuse addr and port */
    ret = setsockopt (fd, SOL_SOCKET,  SO_REUSEADDR, (void *) &on, sizeof (on));
    if (ret < 0)
    {
        close (fd);
        throw APIException((char*)"APIClient::Connect: SO_REUSEADDR failed");
    }
  
    #ifdef SO_REUSEPORT
    ret = setsockopt (fd, SOL_SOCKET, SO_REUSEPORT,
                      (void *) &on, sizeof (on));
    if (ret < 0)
    {
        close (fd);
        throw APIException((char*)"APIClient::Connect: SO_REUSEPORT failed");
    }
  #endif /* SO_REUSEPORT */
  
    /* Prepare address structure for connect */
    memset (&addr, 0, sizeof (struct sockaddr_in));
    memcpy (&addr.sin_addr, hp->h_addr, hp->h_length);
    addr.sin_family = AF_INET;
    addr.sin_port = htons (port);
    //addr.sin_len = sizeof (struct sockaddr_in);
   
    /* Now establish synchronous channel with OSPF daemon */
    ret = connect (fd, (struct sockaddr *) &addr,
                   sizeof (struct sockaddr_in));
    if (ret < 0)
    {
        close (fd);
        snprintf(buf, 128, "APIClient::Connect: connect()%s", strerror (errno));
        throw APIException(buf);
    }

    assert(api_writer);
    api_writer->SetSocket(fd);
}

void APIClient::Connect()
{
    LOG("API Client connecting ... \n");

    assert (_host.size() != 0 && _port != 0);

    if (fd > 0)
    {
        if (api_writer)
            api_writer->Close();
        Close();
    }
    Connect((char*)(_host.c_str()), _port);
}

bool APIClient::IsAlive()
{
    return (fd > 0);
}

// APIClient::ReadMessage is derived from APIReader::ReadMessage()

void APIClient::SendMessage(api_msg *msg)
{
    assert (api_writer);
    api_writer->WriteMessage(msg);
}


////////////////////////////// Global C functions ///////////////////////////
api_msg * api_msg_new (u_int16_t type, u_int16_t length, void *msgbody, u_int32_t ucid, u_int32_t seqnum, u_int32_t tag)
{
    api_msg *msg;

    msg = new (api_msg);
    memset (msg, 0, sizeof(api_msg));

    msg->header.type = htons(type);
    msg->header.length = htons (length);
    msg->header.seqnum = htonl (seqnum);
    msg->header.ucid = htonl(ucid);
    msg->header.tag = htonl(tag);

    if (length > 0 && msgbody != NULL)
    {
        msg->body = new char[length];
        memcpy(msg->body, msgbody, length);
    }
    else 
        msg->body = NULL;

    msg->header.chksum = MSG_CHKSUM(msg->header);
    return msg;
}


struct api_msg * api_msg_append_tlv (struct api_msg * msg, struct api_tlv_header* tlv)
{
    api_msg* rmsg = new (api_msg);
    memcpy(rmsg, msg, sizeof(api_msg));
    rmsg->header.length = htons(ntohs(msg->header.length) + sizeof(api_tlv_header) + ntohs(tlv->length));
    rmsg->header.chksum = MSG_CHKSUM(rmsg->header);
    rmsg->body = new char[ntohs(msg->header.length) + sizeof(api_tlv_header) + ntohs(tlv->length)];    
    memcpy(rmsg->body, msg->body, ntohs(msg->header.length));
    memcpy(rmsg->body + ntohs(msg->header.length), tlv, sizeof(api_tlv_header) + ntohs(tlv->length));
    api_msg_delete(msg);
    return rmsg;
}

void api_msg_delete (struct api_msg* msg)
{
    assert(msg);

    if (msg->body != NULL && ntohs(msg->header.length) > 0)
        delete []msg->body;
    delete msg;
}

void api_msg_dump (struct api_msg* msg)
{
    LOG_DEBUG("api_msg_dump::TYPE(" <<msg->header.type<<") LENGTH("<<ntohs(msg->header.length) <<") SEQ#"<<ntohl(msg->header.seqnum)<<endl);
}

