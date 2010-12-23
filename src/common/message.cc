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

#include <iostream>
#include "utils.hh"
#include "log.hh"
#include "exception.hh"
#include "mxtce.hh"
#include "message.hh"

Message::~Message()
{
    list<TLV*>::iterator itlv;
    for (itlv = tlvList.begin(); itlv != tlvList.end(); itlv++)
        delete (*itlv);
}


void Message::Transmit(int fd)
{
    char buf[MAX_MSG_SIZE];
    memset(buf, 0, MAX_MSG_SIZE);
 
    // Length of message including header
    MessageHeader* header = (MessageHeader*)buf;
    header->type = type;
    header->length = sizeof(MessageHeader);
    strncpy(header->queue, queueName.c_str(), 64);
    strncpy(header->topic, queueName.c_str(), 64);
    if (flagUrgent)
        header->flags |= MSG_FLAG_URGENT;

    list<TLV*>::iterator itlv;
    for (itlv = tlvList.begin(); itlv != tlvList.end(); itlv++)
    {
        TLV* tlv = *itlv;
        memcpy(buf, tlv, tlv->length+4);
        header->length += (tlv->length+4);
    }

    int wlen = writen(fd, buf, header->length);
    std::stringstream ssMsg;
    if (wlen < 0)
    {
        ssMsg << "MessageWriter failed to write " << fd;
        throw MsgIOException(ssMsg.str());
    }
    else if (wlen == 0)
    {
        close(fd);
        ssMsg << "Connection closed for MessageWriter(" << fd << ')';
        throw MsgIOException(ssMsg.str());
    }
    else if (wlen != header->length)
    {
        ssMsg << "MessageWriter(" << fd << ") cannot write the message";
        throw MsgIOException(ssMsg.str());
    }
}


void Message::Receive(int fd)
{
    char buf[MAX_MSG_SIZE];
    memset(buf, 0, MAX_MSG_SIZE);
    MessageHeader* header = (MessageHeader*)buf;
    int msglen;
    int rlen;

    // Read message header 
    rlen = readn (fd, (char*)header, sizeof (MessageHeader));

    std::stringstream ssMsg;
    if (rlen < 0)
    {
        ssMsg << "MessageReader failed to read from "<<fd;
        throw MsgIOException(ssMsg.str());
    }
    else if (rlen == 0)
    {
        close(fd);
        ssMsg << "Connection closed for MessageReader(" << fd <<')';
        throw MsgIOException(ssMsg.str());
    }
    else if (rlen != sizeof (MessageHeader))
    {
        ssMsg << "MessageReader(" << fd << ") cannot read the message header";
        throw MsgIOException(ssMsg.str());
    }

    msglen = header->length;
    if (msglen > MAX_MSG_SIZE)
    {
        ssMsg << "MessageReader(" << fd << ") cannot read oversized packet";
        throw MsgIOException(ssMsg.str());
    }

    if (msglen > sizeof(MessageHeader))
    {
        // Read message body
        rlen = readn (fd, buf, msglen-sizeof(MessageHeader));
        if (rlen < 0)
        {
             ssMsg << "MessageReader failed to read from" << fd;
             throw MsgIOException(ssMsg.str());
        }
        else if (rlen == 0)
        {
             ssMsg << "Connection closed for MessageReader(" << fd <<')';
             throw MsgIOException(ssMsg.str());
        }
        else if (rlen != msglen-sizeof(MessageHeader))
        {
             ssMsg << "MessageReader(" << fd << ") cannot read the message body";
             throw MsgIOException(ssMsg.str());
        }
    }
    this->type = header->type;
    this->queueName = header->queue;
    this->topicName = header->topic;
    this->flagUrgent = !((header->flags & MSG_FLAG_URGENT) == 0);

    int offset = sizeof(MessageHeader);
    while (offset + sizeof(TLV) < msglen)
    {
        TLV* tlv = (TLV*)(buf + offset);
        TLV* newTlv = (TLV*) new char[tlv->length + 4];
        memcpy((char*)newTlv, buf + offset, tlv->length + 4);
        tlvList.push_back(newTlv);
    }
}


void MessageReader::Run()
{
    Reader::Run();

    // read an API message
    if (fd < 0)
        return;

    Message * msg = NULL;

    try {
        msg = ReadMessage();
    } catch (MsgIOException& e) {
       // something is wrong with socket read
       Close();
       throw;
    }

    this->inQueue.push_back(msg);
}


Message* MessageReader::ReadMessage()
{
    Message* msg = new Message(); //empty message
    assert(this->Socket() >= 0);
    try {
        msg->Receive(this->Socket());
    } catch (MsgIOException& e) {
        LOG("MessageReader::ReadMessage caught Exception:" << e.what() << " ErrMsg: " << e.GetMessage() << endl);
        delete msg;
        throw;
    }
    return msg;
}


void MessageWriter::Run()
{
    Writer::Run();

    //write an API message
    if (fd < 0)
        return;

    if (outQueue.empty())
        return;

    Message * msg = outQueue.front();
    assert (msg);
    outQueue.pop_front();

    try {
        WriteMessage(msg); //msg is deleted in WriteMessage
    } catch (MsgIOException e) {
        throw; // ?? comment out ??
    }

    if (outQueue.size() > 0)
    {
        this->SetObsolete(false);
        this->SetRepeats(1);
    }
}


void MessageWriter::WriteMessage(Message* msg)
{
    assert (msg && this->Socket() >= 0);
    try {
        msg->Transmit(this->Socket());
    } catch (MsgIOException& e) {
        LOG("MessageReader::ReadMessage caught Exception:" << e.what() << " ErrMsg: "<< e.GetMessage() << endl);
        throw;
    }
}


void MessagePort::Run()
{
    try {
        MessageReader::Run();
    } catch (MsgIOException& e) {
        eventMaster->Remove(this);
        eventMaster->Remove(this->GetWriter());
    }
    msgRouter->Check();    
}

// pipeName == portName
void MessagePort::AttachPipesAsServer()
{
    int rfd, wfd, ret;
    std::stringstream ssMsg;

    // Create named pipe for reading
    string pipe1 = MxTCE::tmpFilesDir+portName;
    pipe1 += "_pipe_in";
    ret = mkfifo(pipe1.c_str(), 0666);
    if ((ret == -1) && (errno != EEXIST)) {
        ssMsg << "MessagePort::AttachPipesAsServer failed on  mkfifo(" << pipe1<<", 0666)";
        throw MsgIOException(ssMsg.str());
    }
    // Create named pipe for writing
    string pipe2 = MxTCE::tmpFilesDir+portName;
    pipe2 += "_pipe_out";
    ret = mkfifo(pipe2.c_str(), 0666);
    if ((ret == -1) && (errno != EEXIST)) {
        ssMsg << "MessagePort::AttachPipesAsServer failed on  mkfifo(" << pipe2<<", 0666)";
        throw MsgIOException(ssMsg.str());
    }
    // Open named pipe for reading
    rfd = open(pipe1.c_str(), O_RDONLY);
    if (rfd < 0) 
    {
        ssMsg << "MessagePort::AttachPipesAsServer failed on  open(" << pipe1<<", O_RDONLY)";
        throw MsgIOException(ssMsg.str());
    }
    // Open named pipe for writing
    wfd = open(pipe2.c_str(), O_WRONLY);
    if (wfd < 0) 
    {
        ssMsg << "MessagePort::AttachPipesAsServer failed on  open(" << pipe2<<", O_WRONLY)";
        throw MsgIOException(ssMsg.str());
    }
    this->SetSocket(rfd);
    msgWriter.SetSocket(wfd);

    // schedule reader into selector
    msgWriter.SetAutoDelete(false);
    msgWriter.SetRepeats(0);
    this->SetAutoDelete(true);
    this->SetRepeats(FOREVER);
    eventMaster->Schedule(this);
    up = true;
}


// pipeName == portName
void MessagePort::AttachPipesAsClient()
{
    int rfd, wfd, ret;
    std::stringstream ssMsg;

    // Create named pipe for reading
    string pipe1 = MxTCE::tmpFilesDir+portName;
    pipe1 += "_pipe_in";
    string pipe2 =MxTCE::tmpFilesDir+portName;
    pipe2 += "_pipe_out";
    // Open named pipe for writing
    wfd = open(pipe1.c_str(), O_WRONLY);
    if (wfd < 0) 
    {
        ssMsg << "MessagePort::AttachPipesAsServer failed on  open(" << pipe2<<", O_RDONLY)";
        throw MsgIOException(ssMsg.str());
    }
    // Open named pipe for reading
    rfd = open(pipe2.c_str(), O_RDONLY);
    if (rfd < 0) 
    {
        ssMsg << "MessagePort::AttachPipesAsServer failed on  open(" << pipe1<<", O_WRONLY)";
        throw MsgIOException(ssMsg.str());
    }

    this->SetSocket(rfd);
    msgWriter.SetSocket(wfd);

    // schedule reader into selector
    msgWriter.SetAutoDelete(false);
    msgWriter.SetRepeats(0);
    this->SetAutoDelete(true);
    this->SetRepeats(FOREVER);
    eventMaster->Schedule(this);
    up = true;
}


// pipeName == portName
void MessagePort::DetachPipes()
{
    msgWriter.Close();
    this->Close();
    //@@@@ remove named pipe files ?
    up = false;
}


Message* MessagePort::GetMessage ()
{
    if (inQueue.empty())
        return NULL;
    else 
    {
        Message* msg = inQueue.front();
        inQueue.pop_front();
        return msg;
    }
}


void MessagePort::PostMessage (Message *msg)
{
    assert (msg);

    if (msg->IsUrgent())
        msgWriter.outQueue.push_front(msg);
    else
        msgWriter.outQueue.push_back(msg);

    msgWriter.SetRepeats(0);
    msgWriter.SetObsolete(false);
    if (msgWriter.outQueue.size() == 1)
        eventMaster->Schedule(&msgWriter);
    //other msgs in the queue will be handled by MessageWriter
}


///////////////////////////////////////////


Message* MessagePortLoopback::GetMessage ()
{
    if (inQueue.empty())
        return NULL;
    else 
    {
        Message* msg = inQueue.front();
        inQueue.pop_front();
        return msg;
    }
}


void MessagePortLoopback::PostMessage (Message *msg)
{
    assert (msg);

    if (msg->IsUrgent())
        msgWriter.outQueue.push_front(msg);
    else
        msgWriter.outQueue.push_back(msg);

    if (msgWriter.outQueue.size() > 0)
        mxTCE->CheckMessage();
}


Message* MessagePortLoopback::GetLocalMessage ()
{
    if (msgWriter.outQueue.empty())
        return NULL;
    else 
    {
        Message* msg = msgWriter.outQueue.front();
        msgWriter.outQueue.pop_front();
        return msg;
    }
}


void MessagePortLoopback::PostLocalMessage (Message *msg)
{
    assert (msg);

    if (msg->IsUrgent())
        inQueue.push_front(msg);
    else
        inQueue.push_back(msg);

    msgRouter->Check();
}


///////////////////////////////////////////

MessageRouter MessageRouter::router;

MessageRouter::MessageRouter()
{
    up = false;
    eventMaster = NULL;
}


MessageRouter::~MessageRouter()
{
    while (!msgPortList.empty())
    {
        delete msgPortList.front();
        msgPortList.pop_front();
    }

    while (!routeList.empty())
    {
        delete routeList.front();
        routeList.pop_front();
    }
}


void MessageRouter::Start()
{
    // loop portList and attach it to pipe then schedule it
    list<MessagePort*>::iterator it = msgPortList.begin();
    for (; it != msgPortList.end(); it++)
    {
        MessagePort* msgPort = *it;
        try {
            msgPort->AttachPipesAsServer();
        } catch (MsgIOException& e) {
            LOG("MessageRouter::Start caught" << e.what() << " when attaching port: " << msgPort->GetName() << " ErrMesage:" << e.GetMessage() << endl);
            //escalate the exception
            throw;
        }
    }
    this->SetAutoDelete(false);
    this->SetRepeats(0);
    eventMaster->Schedule(this);    
    up = true;
}


void MessageRouter::Run()
{
    //msg routing and forwarding
    // Loop through portList
    list<MessagePort*>::iterator itPort;
    // $$$$ TODO : Handle MxTCE core bound messages
    for (itPort = msgPortList.begin(); itPort != msgPortList.end(); itPort++)
    {
        MessagePort* msgPort = *itPort;
        if (!msgPort || !msgPort->IsUp())
            continue;
        Message* msg;
        Route* route;
        // $$$$ TODO : Handle "delivery confirmation" and "resend" in GetMessage()
        while ((msg = msgPort->GetMessage()) != NULL)
        {
            route = LookupRoute(msg->GetQueue(), msg->GetTopic());
            if (route)
            {
                list<string>& portNameList = route->GetPortList();
                if (portNameList.empty())
                    continue;
                list<MessagePort*> outPortList;
                list<MessagePort*>::iterator itP;
                if (portNameList.front() == "ALL")
                {
                    for (itP = msgPortList.begin(); itP != msgPortList.end(); itP++)
                    {
                        MessagePort* msgOutPort = *itP;
                        if (msgOutPort&& msgOutPort->IsUp())
                            outPortList.push_back(msgOutPort);
                    }
                }
                else 
                {
                    list<string>::iterator itN = portNameList.begin();
                    for (itN =portNameList.begin(); itN != portNameList.end(); itN++)
                    {
                        string portName = *itN;
                        for (; itP != msgPortList.end(); itP++) 
                        {
                            MessagePort* msgOutPort = *itP;
                            if (msgOutPort && msgOutPort->IsUp() && msgOutPort->GetName() == portName)
                                outPortList.push_back(msgOutPort);
                        }
                    }
                }
                for (itP = outPortList.begin(); itP != outPortList.end(); itP++)
                {
                    MessagePort* msgOutPort = *itP;
                    msgOutPort->PostMessage(msg);
                }
            }
        }
    }
}


void MessageRouter::Check()
{
    // activate the router event and make sure the router event is scheduled
    list<Event*> eventList = eventMaster->GetEventList(EVENT_PRIORITY);
    list<Event*>::iterator itE = eventList.begin();
    for (; itE != eventList.end(); itE++)
    {
        Event* event = *itE;
        if ((MessageRouter*)event == this)
        {
            return;
        }
    }
    this->SetRepeats(0);
    eventMaster->Schedule(this);
}


MessagePort* MessageRouter::AddPort(string& portName)
{
    // new port,
    MessagePort* msgPort = new MessagePort(portName, this);
    assert(eventMaster);
    msgPort->SetEventMaster(eventMaster);
    msgPortList.push_back(msgPort);
    //attach to pipe and scheule if MessageRouter has already started
    if (this->up) 
    {
        try {
            msgPort->AttachPipesAsServer();
        } catch (MsgIOException& e) {
            LOG("MessageRouter::AddPort caught" << e.what() << " when attaching port: " << msgPort->GetName() << " ErrMesage:" << e.GetMessage() << endl);
            //escalate the exception
            throw;
        }
    }
    return msgPort;
}


MessagePort* MessageRouter::LookupPort(string& portName)
{
    list<MessagePort*>::iterator it = msgPortList.begin();
    for (; it != msgPortList.end(); it++)
    {
        MessagePort* msgPort = *it;
        if (msgPort->GetName() == portName)
            return msgPort;
    }
    return NULL;
}


void MessageRouter::DeletePort(string& portName)
{
    if (this->up)
    {
        list<MessagePort*>::iterator it = msgPortList.begin();
        for (; it != msgPortList.end(); it++)
        {
            MessagePort* msgPort = *it;
            if (msgPort->GetName() == portName)
            {
                msgPortList.erase(it);
                delete msgPort;
                return;
            }
        }
    }
}


Route* MessageRouter::AddRoute(string& queueName, string& topicName, string& portName)
{
    Route* route = new Route(queueName, topicName, portName);
    routeList.push_back(route);
    return route;
    
}


Route* MessageRouter::LookupRoute(string& queueName, string& topicName)
{
    list<Route*>::iterator it = routeList.begin();
    for (; it != routeList.end(); it++)
    {
        Route* route = *it;
        if (route->GetQueue() == queueName && route->GetTopic() == topicName)
        {
            return route;
        }
    }
    return NULL;
}


void MessageRouter::DeleteRoute(string& queueName, string& topicName)
{
    list<Route*>::iterator it = routeList.begin();
    for (; it != routeList.end(); it++)
    {
        Route* route = *it;
        if (route->GetQueue() == queueName && route->GetTopic() == topicName)
        {
            routeList.erase(it);
            delete route;
            return;
        }
    }
}


