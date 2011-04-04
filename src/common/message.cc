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

#include <iostream>
#include "utils.hh"
#include "log.hh"
#include "exception.hh"
#include "mxtce.hh"
#include "message.hh"

list<MessagePipe*> MessagePipeFactory::pipes;
Lock MessagePipeFactory::mpfLock;

Message::~Message()
{
    list<TLV*>::iterator itlv;
    for (itlv = tlvList.begin(); itlv != tlvList.end(); itlv++)
        delete (*itlv);
}

Message* Message::Duplicate()
{
    Message* msg = new Message((MessageType)this->type, this->queueName, this->topicName);
    msg->flagUrgent = this->flagUrgent;
    bool flagUrgent;
    msg->port = this->port;
    list<TLV*>::iterator itlv; 
    for (itlv = this->tlvList.begin(); itlv != this->tlvList.end(); itlv++)
    {
        TLV* tlv = (TLV*)(new char[TLV_HEAD_SIZE + (*itlv)->length]);
        memcpy(tlv, (char*)(*itlv), TLV_HEAD_SIZE + (*itlv)->length);
        msg->tlvList.push_back(tlv);
    }
    return msg;
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
    strncpy(header->topic, topicName.c_str(), 64);
    if (flagUrgent)
        header->flags |= MSG_FLAG_URGENT;

    list<TLV*>::iterator itlv;
    for (itlv = tlvList.begin(); itlv != tlvList.end(); itlv++)
    {
        TLV* tlv = *itlv;
        memcpy(buf+header->length, tlv, TLV_HEAD_SIZE+tlv->length);
        header->length += (TLV_HEAD_SIZE+tlv->length);
    }

    int wlen = writen(fd, buf, header->length);
    if (wlen < 0)
    {
        snprintf(buf, 128, "MessageWriter failed to write %d", fd);
        throw MsgIOException(buf);
    }
    else if (wlen == 0)
    {
        close(fd);
        snprintf(buf, 128, "Connection closed for MessageWriter(%d)", fd);
        throw MsgIOException(buf);
    }
    else if (wlen != header->length)
    {
        snprintf(buf, 128,  "MessageWriter(%d) cannot write the message", fd);
        throw MsgIOException(buf);
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

    if (rlen < 0)
    {
        snprintf(buf, 128, "MessageReader failed to read from %d", fd);
        throw MsgIOException(buf);
    }
    else if (rlen == 0)
    {
        close(fd);
        snprintf(buf, 128, "Connection closed for MessageReader(%d)", fd);
        throw MsgIOException(buf);
    }
    else if (rlen != sizeof (MessageHeader))
    {
        snprintf(buf, 128, "MessageReader(%d) cannot read the message header", fd);
        throw MsgIOException(buf);
    }

    msglen = header->length;
    if (msglen > MAX_MSG_SIZE)
    {
        snprintf(buf, 128, "MessageReader(%d) cannot read oversized packet", fd);
        throw MsgIOException(buf);
    }

    if (msglen > sizeof(MessageHeader))
    {
        // Read message body
        rlen = readn (fd, buf+sizeof(MessageHeader), msglen-sizeof(MessageHeader));
        if (rlen < 0)
        {
            snprintf(buf, 128, "MessageReader failed to read from %d", fd);
            throw MsgIOException(buf);
        }
        else if (rlen == 0)
        {
            snprintf(buf, 128, "Connection closed for MessageReader(%d)", fd);
            throw MsgIOException(buf);
        }
        else if (rlen != msglen-sizeof(MessageHeader))
        {
            snprintf(buf, 128, "MessageReader(%d) cannot read the message body", fd);
            throw MsgIOException(buf);
        }
    }
    this->type = header->type;
    this->queueName = header->queue;
    this->topicName = header->topic;
    this->flagUrgent = !((header->flags & MSG_FLAG_URGENT) == 0);

    int offset = sizeof(MessageHeader);
    while (offset + TLV_HEAD_SIZE < msglen)
    {
        TLV* tlv = (TLV*)(buf + offset);
        TLV* newTlv = (TLV*) new char[TLV_HEAD_SIZE+tlv->length];
        memcpy((char*)newTlv, buf + offset, TLV_HEAD_SIZE+tlv->length);
        tlvList.push_back(newTlv);
        offset += (TLV_HEAD_SIZE+tlv->length);
    }
}


void Message::LogDump()
{
    LOG_DEBUG("Message Dump [type=" << this->type << " to queue=" << this->queueName << " with topic=" << this->topicName
        << " urgentFlag=" << (this->flagUrgent?"true":"false") << " TLV count=" << this->tlvList.size() << "]" <<endl); 
    if (port != NULL)
        LOG_DEBUG("    ==> MessagePort " << port->GetName() << " on Socket " << port->Socket() << endl);
}


////////////////////////////


void MessageReader::Run()
{
    Reader::Run();

    // read an API message
    if (fd < 0)
        return;

    Message * msg = NULL;

    msg = ReadMessage();

    this->inQueue.push_back(msg);
}


Message* MessageReader::ReadMessage()
{
    Message* msg = new Message(); //empty message
    assert(this->Socket() >= 0);
    try {
        msg->Receive(this->Socket());
    } catch (MsgIOException& e) {
        LOG("MessageReader::ReadMessage exception: " << e.what() << endl);
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
        LOG("MessageReader::ReadMessage exception: " << e.what() << endl);
        throw;
    }
    delete msg; //message consumed
}


void MessagePort::Run()
{
    try {
        MessageReader::Run();
    } catch (MsgIOException& e) {
        Close();
        return;
    }
    if (threadScheduler)
        threadScheduler->hookHandleMessage();
    if (msgRouter)
        msgRouter->Check();    
}


void MessagePort::Close()
{
    if (eventMaster != NULL)
    {
        eventMaster->Remove(&this->msgWriter);
        eventMaster->Remove(this);
        this->msgWriter.Close();
        Selector::Close();
        this->msgWriter.SetRepeats(0);
        this->SetRepeats(0);
    }
    up = false;
}


// pipeName == portName
void MessagePort::AttachPipes()
{
    char buf[128];

    msgWriter.SetSocket(this->Socket());

    // schedule reader into selector
    msgWriter.SetAutoDelete(false);
    msgWriter.SetRepeats(0);
    this->SetAutoDelete(false);
    this->SetRepeats(FOREVER);
    eventMaster->Schedule(this);
    up = true;
}


// pipeName == portName
void MessagePort::DetachPipes()
{
    this->Close();
    if (this->msgRouter)
        this->msgRouter->DeletePort(portName);
}


Message* MessagePort::GetMessage ()
{
    if (Socket() < 0 || inQueue.empty())
        return NULL;
    else 
    {
        Message* msg = inQueue.front();
        inQueue.pop_front();
        if (!msg->GetPort())
            msg->SetPort(this);
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


// message router retrieve message from the loopback port
Message* MessagePortLoopback::GetMessage ()
{
    if (inQueue.empty())
        return NULL;
    else 
    {
        Message* msg = inQueue.front();
        inQueue.pop_front();
        if (!msg->GetPort())
            msg->SetPort(this);
        return msg;
    }
}


// message router deliver message to the loopback port
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


// core thread get message from the message router ( from other ports)
Message* MessagePortLoopback::GetLocalMessage ()
{
    if (msgWriter.outQueue.empty())
        return NULL;
    else 
    {
        Message* msg = msgWriter.outQueue.front();
        msgWriter.outQueue.pop_front();
        if (!msg->GetPort())
            msg->SetPort(this);
        return msg;
    }
}


// core thread send message to the message router( then to other ports)
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
            msgPort->AttachPipes();
            LOG("MessageRouter::Start AttachPipes on server side" << endl);
        } catch (MsgIOException& e) {
            LOG("MessageRouter::Start caught" << e.what() << " when attaching port: " << msgPort->GetName() << endl);
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
        // TODO: Routing can be faster if using two-level hash map:  {queue, topic} ->portlist
        // TODO: When passing duplicate messages, watching out for thread-safe operations on passed pointers. Dup pointed data if needed!
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
                    for (itP                                                                                                                                                       = msgPortList.begin(); itP != msgPortList.end(); itP++)
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
                        for (itP = msgPortList.begin(); itP != msgPortList.end(); itP++) 
                        {
                            MessagePort* msgOutPort = *itP;
                            if (msgOutPort && msgOutPort->IsUp() && msgOutPort->GetName() == portName)
                                outPortList.push_back(msgOutPort);
                        }
                    }
                }
                if (outPortList.empty())
                {
                    LOG("MessageRouter found am empty route for delivering msg (type:" << msg->GetType() 
                        << ") to queue: " << msg->GetQueue() << " with topic: " << msg->GetTopic() << endl);
                    continue;
                }
                for (itP = outPortList.begin(); itP != outPortList.end(); itP++)
                {
                    MessagePort* msgOutPort = *itP;
                    if (itP != outPortList.begin())
                        msg = msg->Duplicate();
                    msgOutPort->PostMessage(msg);
                }
            }
            else 
            {
                LOG("MessageRouter cannot find a route for delivering msg (type:" << msg->GetType() 
                    << ") to queue: " << msg->GetQueue() << " with topic: " << msg->GetTopic() << endl);
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
    try {
       MessagePipeFactory::CreateMessagePipe(portName, this);
    } catch (MsgIOException e) {
        LOG("MessageRouter::AddPort(" << portName <<") caught exception: " << e.what() << endl);
        return NULL;
    }
    MessagePort* msgPort = MessagePipeFactory::LookupMessagePipe(portName)->GetServerPort();
    assert(msgPort);
    assert(eventMaster);
    msgPort->SetEventMaster(eventMaster);
    msgPortList.push_back(msgPort);
    //attach to pipe and scheule if MessageRouter has already started
    if (this->up) 
    {
        try {
            msgPort->AttachPipes();
            LOG("MessageRouter::AddPort CreateMessagePipe and call AttachPipes on server side" << endl);
        } catch (MsgIOException& e) {
            LOG("MessageRouter::AddPort caught" << e.what() << " when attaching port: " << msgPort->GetName() << endl);
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
    list<MessagePort*>::iterator it = msgPortList.begin();
    for (; it != msgPortList.end(); it++)
    {
        MessagePort* msgPort = *it;
        if (msgPort->GetName() == portName)
        {
            msgPortList.erase(it);
            if ((*it)->IsUp())
                (*it)->DetachPipes();
            return;
        }
    }
}


Route* MessageRouter::AddRoute(string& queueName, string& topicName, string& portName)
{
    Route* route = LookupRoute(queueName, topicName);
    if (route) 
    {
        route->AddPortName(portName);
    }
    else
    {
        route = new Route(queueName, topicName, portName);
        routeList.push_back(route);
    }
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


void MessagePipe::InitPipe()
{
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
        char buf[128];
        snprintf(buf, 128, "MessagePipe(%s) failed to init sockets.", name.c_str());
        throw MsgIOException(buf);
    }
    serverPort->SetSocket(sockets[0]);
    clientPort->SetSocket(sockets[1]);
}


MessagePipe* MessagePipeFactory::CreateMessagePipe(string name, MessageRouter* router) 
{
    mpfLock.DoLock();
    MessagePipe* pipe = new MessagePipe(name, router);
    pipes.push_back(pipe);
    mpfLock.Unlock();
    return pipe;
}


MessagePipe* MessagePipeFactory::LookupMessagePipe(string name) 
{
    mpfLock.DoLock();
    list<MessagePipe*>::iterator itp;
    for (itp = pipes.begin(); itp != pipes.end(); itp++)
    {
        if ((*itp)->GetName() == name)
        {
            mpfLock.Unlock();
            return (*itp);
        }
    }
    mpfLock.Unlock();
    return NULL;
}


void MessagePipeFactory::RemoveMessagePipe(string name) 
{
    mpfLock.DoLock();
    list<MessagePipe*>::iterator itp;
    for (itp = pipes.begin(); itp != pipes.end(); itp++)
    {
        if ((*itp)->GetName() == name)
        {
            pipes.erase(itp);
            mpfLock.Unlock();
            return;
        }
    }
    mpfLock.Unlock();
}    


