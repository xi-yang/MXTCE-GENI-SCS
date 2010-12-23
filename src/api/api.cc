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
#include "api.hh"
#include "utils.hh"
#include "log.hh"


// TODO: Exception handling

int APIServer::Start()
{
    int sock;
    int ret = -1;
    struct sockaddr_in sa_in;
    memset (&sa_in, 0, sizeof (struct sockaddr_in));

    //Get a stream socket
    sock = socket (AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        LOG("can't make socket sockunion_stream_socket");
        return ret;
    }
  
    // reuse address and port on the server
    int on = 1;
    ret = setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (void *) &on, sizeof (on));
    if (ret < 0)
    {
        LOGF("can't set sockopt SO_REUSEADDR to socket %d", sock);
        return ret;
    }
#ifdef SO_REUSEPORT
    ret = setsockopt (sock, SOL_SOCKET, SO_REUSEPORT, (void *) &on, sizeof (on));
    if (ret < 0)
    {
        LOGF("can't set sockopt SO_REUSEPORT to socket %d", sock);
        return ret;
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
      LOGF("can't bind socket : %s", strerror (errno));
      close (sock);
      return ret;
    }

    // Listen under queue length 10
    ret = listen (sock, 9);
    if (ret < 0)
    {
      LOGF ("APIServer::Init()::listen: %s",  strerror (errno));
      close (sock);
      return ret;
    }

    //Sechedule itself
    this->fd = sock;
    this->SetRepeats(FOREVER);
    eventMaster->Schedule(this);

    return ret;
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
        LOGF("APIServer::Run() Cannot accept socket on %d\n", fd);
        return;
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

    api_msg * msg = ReadMessage();

    //something is wrong with socket read
    if (!msg)
    {
        assert(api_writer);
        api_writer->Close();
        Close();
        return;
    }

    //api_msg_dump(msg);

    //Message handling by using APIServer callback
    server->HandleMessage(this, this->api_writer, msg); 
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
        LOG("APIReader failed to read from "<<fd<<endl);
        return NULL;
    }
    else if (rlen == 0)
    {
        close(fd);
        LOG("Connection closed for APIReader(" << fd <<')' << endl);
        return NULL;
    }
    else if (rlen != sizeof (struct api_msg_header))
    {
        LOG("APIReader(" << fd << ") cannot read the message header" <<endl);
        return NULL;
    }

    if (MSG_CHKSUM(header) != header.chksum)
    {
        LOGF("APIReader(%d) packet corrupt (ucid=0x%x, seqno=0x%x).\n", fd, ntohl(header.ucid), ntohl(header.seqnum));
        return NULL;
    }

    // Determine body length. 
    bodylen = ntohs (header.length);
    if (bodylen > API_MAX_MSG_SIZE)
    {
        LOG("APIReader(" << fd << ") cannot read oversized packet" <<endl);
        return NULL;
    }

    if (bodylen > 0)
    {
        // Read message body
        rlen = readn (fd, buf, bodylen);
        if (rlen < 0)
        {
             LOG("APIReader failed to read from" << fd << endl);
            return NULL;
        }
        else if (rlen == 0)
        {
             LOG("Connection closed for APIReader(" << fd <<')' << endl);
            return NULL;
        }
        else if (rlen != bodylen)
        {
             LOG("APIReader(" << fd << ") cannot read the message body" <<endl);
            return NULL;
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
    int ret = WriteMessage(msg); //msg is deleted in WriteMessage

    //something is wrong with socket write
    if (ret < 0)
    {
        assert(api_reader);
        api_reader->Close();
        Close();
        return;
    }

    if (msgQueue.size() > 0)
    {
        this->SetObsolete(false);
        this->SetRepeats(1);
    }
}

int APIWriter::WriteMessage (api_msg *msg)
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
        LOG("APIWriter(" << fd << ") packet corrupt" <<endl);
        api_msg_delete (msg);
        return -1;
    }

    api_msg_delete (msg);

    int wlen = writen(fd, buf, len);
    if (wlen < 0)
    {
        LOG("APIWriter failed to write " << fd << endl);
        return -1;
    }
    else if (wlen == 0)
    {
        close(fd);
        LOG("Connection closed for APIWriter(" << fd <<')' << endl);
        return -1;
    }
    else if (wlen != len)
    {
        LOG("APIWriter(" << fd << ") cannot write the message" <<endl);
        return -1;
    }
    return 0;
}

void APIWriter::PostMessage (api_msg *msg, bool urgent)
{
    assert (msg);

    if (urgent == URGENT_POST)
        this->msgQueue.push_front(msg);
    else
        this->msgQueue.push_back(msg);

    this->SetRepeats(0);
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

int APIClient::Connect(char *host, int port)
{
      struct sockaddr_in addr;
      struct hostent *hp;
      int ret;
      int size;
      int on = 1;

    _host = host;
    _port = port;
  
      assert (strlen(host) > 0 || port > 0);
  	
      hp = gethostbyname (host);
      if (!hp)
      {
          fprintf (stderr, "APIClient::Connect: no such host %s\n", host);
          return (-1);
      }
  
      fd = socket (AF_INET, SOCK_STREAM, 0);
      if (fd < 0)
      {
  	  LOGF("APIClient::Connect: socket(): %s", strerror (errno));
  	  return (-1);
      }
                                                                                 
      /* Reuse addr and port */
      ret = setsockopt (fd, SOL_SOCKET,  SO_REUSEADDR, (void *) &on, sizeof (on));
      if (ret < 0)
      {
          fprintf (stderr, "APIClient::Connect: SO_REUSEADDR failed\n");
          close (fd);
          return (-1);
      }
  
  #ifdef SO_REUSEPORT
    ret = setsockopt (fd, SOL_SOCKET, SO_REUSEPORT,
                      (void *) &on, sizeof (on));
    if (ret < 0)
    {
        fprintf (stderr, "APIClient::Connect: SO_REUSEPORT failed\n");
        close (fd);
        return (-1);
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
        LOGF("APIClient::Connect: connect(): %s", strerror (errno));
        close (fd);
        return (-1);
    }

    assert(api_writer);
    api_writer->SetSocket(fd);
    return fd;
}

int APIClient::Connect()
{
    LOG("API Client connecting ... \n");

    if (_host.size() == 0 || _port == 0)
        return -1;

    if (fd > 0)
    {
        if (api_writer)
            api_writer->Close();
        Close();
    }
    if (Connect((char*)(_host.c_str()), _port) < 0)
    {
        return -1;
    }

    return 0;
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

