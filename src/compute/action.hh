/*
 * Copyright (c) 2010-2011
 * ARCHSTONE Project.
 * University of Southern California/Information Sciences Institute.
 * All rights reserved.
 *
 * Created by Xi Yang 2011
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
 
#ifndef __ACTION_HH__
#define __ACTION_HH__
#include <list>
#include "types.hh"
#include "event.hh"
#include "message.hh"

using namespace std;

typedef enum {
    _Idle = 0,
    _Started = 1,
    _WaitingMessages,
    _WaitingChildren,
    _Cancelled,
    _Failed,
    _Finished
} ActionState;

class ComputeWorker;
class Action: public Timer
{
protected:
    string context;
    string name;
    ActionState state;
    ComputeWorker* worker;
    Action* parent;
    // children actions are scheduled to run in event queue. this allows for concurrent branches
    list<Action*> children; 
    // messages received (as reply) from other threads
    list<Message*> messages;
    // expected message topics. this allows action pending on message replies
    list<string> expectMesssageTopics;

public:
    //use Timer as base class to cover both regular (PRIORITY) and scheduling (TIMER) cases.
    Action(): state(_Idle), worker(NULL), parent(NULL) { SetType(EVENT_PRIORITY); SetNice(true); }
    Action(ComputeWorker* w): state(_Idle), worker(w), parent(NULL) { assert(w != NULL); SetType(EVENT_PRIORITY); SetNice(true); }
    Action(string& n, ComputeWorker* w): name(n), state(_Idle), worker(w), parent(NULL) { assert(w != NULL); SetType(EVENT_PRIORITY);  SetNice(true); }
    Action(string &c, string& n, ComputeWorker* w): context(c), name(n), state(_Idle), worker(w), parent(NULL) { assert(w != NULL); SetType(EVENT_PRIORITY);  SetNice(true); }
    virtual ~Action() { }
    string& GetName() { return name; }
    void SetName(string& n) { name = n; }
    string& GetContext() { return context; }
    void SetContext(string& c) { context = c; }
    ActionState GetState() { return state; }
    void SetState(ActionState s) { this->state = s; }
    ComputeWorker* GetComputeWorker() { return worker; }
    void SetComputeWorker(ComputeWorker* w) { worker = w; }
    list<Message*>& GetMessages() { return messages; }
    list<string>& GetExpectMessageTopics() { return expectMesssageTopics; }
    Action* GetParent() { return parent; } 
    void SetParent(Action* p) { parent = p; }
    list<Action*>& GetChildren() { return children; } 
    void AddChild(Action* child) { child->SetComputeWorker(worker);  child->SetParent(this); children.push_back(child); } 
    void SendMessage(MessageType type, string& queue, string& topic, list<TLV*>& tlvs);
    void SendMessage(MessageType type, string& queue, string& topic, list<TLV*>& tlvs, string& expectReturnTopic);
    virtual void* GetData(string& dataName) {}

    virtual void Run();
    virtual void Wait();
    virtual void Process();
    virtual bool ProcessChildren();
    virtual bool ProcessMessages();
    virtual void Finish();
    virtual void CleanUp();
};


#endif
