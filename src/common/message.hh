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
      
#ifndef __MESSAGE_HH__
#define __MESSAGE_HH__

#include<list>
#include "types.hh"
#include "thread.hh"
#include "event.hh"

using namespace std;

#define MAX_MSG_SIZE 4096

typedef enum {
    MSG_REQ = 0x0001,
    MSG_REPLY,
    MSG_ACK,
    MSG_NACK,
    MSG_ERR
} MessageType;

//no need for hton and htoh conversion
typedef struct {
    u_int16_t type;
    u_int16_t length;
    char queue[64];
    char topic[64];
    char context[64];
    u_int32_t flags;
} MessageHeader;

typedef enum {
    MSG_TLV_VOID_PTR = 0x0001,
    MSG_TLV_API_REQ_PTR = 0x0002,
    MSG_TLV_PATH_INFO = 0x0003,
    MSG_TLV_ERROR_INFO = 0x0004,
    MSG_TLV_API_TAGS = 0x0010,
    MSG_TLV_RESV_INFO = 0x0011,
    MSG_TLV_PATH_ELEM = 0x0012,
} TLVType;

//no need for hton and htoh conversion
typedef struct {
    u_int16_t type;
    u_int16_t length; //value size 
    u_int8_t value[4];
} TLV;
#define TLV_HEAD_SIZE 4

#define MSG_FLAG_URGENT 0x00000001

// Inter-thread message 
class MessagePort;
class Message
{
protected:
    u_int16_t type;
    string queueName;
    string topicName;
    string contextTag;
    bool flagUrgent;
    list<TLV*> tlvList;
    MessagePort* port;

public:
    Message(): type(0), queueName(""), topicName(""), contextTag(""), port(NULL) { }
    Message(MessageType ty, string& qn, string& tn): type(ty), queueName(qn), topicName(tn), contextTag(""), port(NULL) 
        { flagUrgent = false; }
    virtual ~Message();
    u_int16_t GetType() { return type; }
    void SetType(u_int16_t ty) { this->type = ty; }
    string& GetQueue() { return queueName; }
    void SetQueue(string& qn) { queueName = qn; }
    string& GetTopic() { return topicName; }
    void SetTopic(string& tn) { topicName = tn; }
    string& GetContextTag() { return contextTag; }
    void SetContextTag(string& ct) { contextTag = ct; }
    MessagePort* GetPort() { return port; }
    void SetPort(MessagePort* p) { port = p; }
    bool IsUrgent() { return flagUrgent; }
    void SetUrgent(bool ur) { flagUrgent = ur; }
    void AddTLV(TLV* tlv) { tlvList.push_back(tlv); }
    list<TLV*>& GetTLVList() { return tlvList; }
    Message* Duplicate();
    void Transmit(int wfd);
    void Receive(int rfd);
    void LogDump();
};


// MessageReader/Writer pair makes a server-side thread port in MessageQueue
class MessageReader: public Reader 
{
protected:
    list<Message *> inQueue;
    
public:
    MessageReader(int fd):Reader(fd) { }
    virtual ~MessageReader() {}
    virtual void Run ();
    Message * ReadMessage();

    friend class MessagePort;
    friend class MessagePortLoopback;
    friend class MessageRouter;
};


class MessagePort;
class MessageWriter: public Writer 
{
protected:
    MessagePort* portPtr;
    list<Message*> outQueue;

public:
    MessageWriter(int fd): Writer(fd) { }
    virtual ~MessageWriter() {}
    virtual void Run ();
    MessagePort* GetMessagePort() { return portPtr; }    
    void SetMessagePort(MessagePort* po) { portPtr = po; }
    void WriteMessage(Message *msg);

    friend class MessagePort;
    friend class MessagePortLoopback;
    friend class MessageRouter;
};


class MessageRouter;
class ThreadPortScheduler;
class MessagePort: public MessageReader
{
protected:
    bool up;
    string portName;
    MessageWriter msgWriter;
    //this (self) is *the* msgReader
    MessageRouter* msgRouter;
    EventMaster* eventMaster;
    ThreadPortScheduler* threadScheduler;

public:
    MessagePort(string& name): MessageReader(0), portName(name), msgWriter(0), msgRouter(NULL), threadScheduler(NULL), up(false) {  }
    MessagePort(string& name, MessageRouter* router): MessageReader(0), portName(name), msgWriter(0), msgRouter(router), threadScheduler(NULL), up(false) { }
    virtual ~MessagePort() { }
    string& GetName() { return portName; }
    void SetName(string& name) { portName = name; }
    bool IsUp() { return up; }
    void SetEventMaster(EventMaster* em) { eventMaster = em; } 
    MessageWriter* GetWriter() { return &msgWriter; }  
    list<Message*>& GetMsgInQueue() { return inQueue; }
    list<Message*>& GetMsgOutQueue() { return msgWriter.outQueue; }
    ThreadPortScheduler* GetThreadScheduler() { return threadScheduler; }
    void SetThreadScheduler(ThreadPortScheduler* scheduler) { threadScheduler = scheduler; }
    virtual void Run ();
    virtual void Close();
    virtual void AttachPipes();
    virtual void DetachPipes();
    virtual Message* GetMessage();
    virtual void PostMessage (Message *msg);

    friend class MessageRouter;
};


class MxTCE;
class MessagePortLoopback: public MessagePort
{
private:
    MxTCE* mxTCE;

public:
    MessagePortLoopback(string& name, MxTCE* tce):MessagePort(name), mxTCE(tce) { }
    MessagePortLoopback(string& name, MessageRouter* router, MxTCE* tce):MessagePort(name, router), mxTCE(tce) { }
    virtual ~MessagePortLoopback() { }
    virtual void Run () { } //no op
    virtual void AttachPipes() { up = true; }
    virtual void DetachPipes() { up = false; }
    virtual Message* GetMessage();
    virtual void PostMessage (Message *msg);
    virtual Message* GetLocalMessage();
    virtual void PostLocalMessage (Message *msg);
};


class Route
{
protected:
    string queueName;
    string topicName;
    list<string> portNameList; //default: string[0] = "ALL";

public:
    Route(string& qn, string& tn, string& pn): queueName(qn), topicName(tn) 
        { portNameList.push_front(pn); }
    virtual ~Route() { }

    string& GetQueue() { return queueName; }
    string& GetTopic() { return topicName; }
    list<string>& GetPortList() { return portNameList; }
    void SetQueue(string& qn) { queueName = qn; }
    void SetTopic(string& tn) { topicName = tn; }
    void ClearPorNametList() { portNameList.clear(); }
    void AddPortName(string& pn) { portNameList.push_back(pn); }

    friend class MessageRouter;
};


// routing based on Queue + Toppic
class MessageRouter: public Event
{
protected:
    bool up;
    // Port list
    list<MessagePort*> msgPortList;
    // Route list --> hash map ??
    list<Route*> routeList;
    static MessageRouter router;
    EventMaster* eventMaster;

public:
    MessageRouter();
    virtual ~MessageRouter();
    static MessageRouter* GetInstance() { return &MessageRouter::router; }
    EventMaster* GetEventMaster() { return eventMaster; }
    void SetEventMaster(EventMaster* em) { this->eventMaster = em; }
    list<MessagePort*>& GetMessagePortList() { return msgPortList; }
    list<Route*>& GetRouteList() { return routeList; }
    bool IsUp() { return up; }
    void Start();
    virtual void Run ();
    void Check();
    MessagePort* AddPort(string& portName);
    MessagePort* LookupPort(string& portName);
    void DeletePort(string& portName);
    Route* AddRoute(string& queueName, string& topicName, string& portName);
    Route* LookupRoute(string& queueName, string& topicName);
    void DeleteRoute(string& queueName, string& topicName);
};


class MessagePipe
{
private:
    string name;
    int sockets[2];//sockets[0]: server side; sockets[1] client side
    MessagePort* serverPort;
    MessagePort* clientPort;
    
    MessagePipe() { }
    void InitPipe();

public:
    MessagePipe(string n): name(n) 
    {
        serverPort = new MessagePort(n);
        clientPort = new MessagePort(n);
        InitPipe();
    }
    MessagePipe(string n, MessageRouter* r): name(n) 
    {
        serverPort = new MessagePort(n, r);
        clientPort = new MessagePort(n);
        InitPipe();
    }
    ~MessagePipe()
    {
        if (serverPort->IsUp())
            serverPort->DetachPipes();
        if (clientPort->IsUp())
            clientPort->DetachPipes();
        delete serverPort;
        delete clientPort;
    }
    string GetName() { return name; }
    MessagePort* GetServerPort() { return serverPort; }
    MessagePort* GetClientPort() { return clientPort; }
};


class Lock;
class MessagePipeFactory
{
private:
    static list<MessagePipe*> pipes;
    static Lock mpfLock;

public:
    static MessagePipe* CreateMessagePipe(string name, MessageRouter* router);
    static MessagePipe* LookupMessagePipe(string name);
    static void RemoveMessagePipe(string name);
};


#endif
