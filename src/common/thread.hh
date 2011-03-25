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


#ifndef __THREAD_HH__
#define __THREAD_HH__

#include <pthread.h>
#include "types.hh"
#include "event.hh"
#include "message.hh"
#include "exception.hh"
#include "log.hh"


using namespace std;

class Runnable 
{ 
public:
    virtual void* Run() = 0; 
    virtual ~Runnable() = 0;
}; 

class Thread 
{ 
private:
    pthread_t _id;  // thread ID 
    bool detached; // true if thread created in detached state; false otherwise
    pthread_attr_t threadAttribute;
    auto_ptr<Runnable> runnable;
    void* arg;
    void* result;	// stores return value of run() 
    string nameTag;

    // Prevent copying or assignment
    Thread(const Thread&);
    Thread& operator= (const Thread&);

    static void* ExecRunnable(void* pVoid);
    static void* Exec(void* pVoid); 

public:
    Thread(auto_ptr<Runnable> runnable_, bool isDetached = false) {
        assert(runnable.get() != NULL);
        _id = 0;
        arg = NULL;
        result = NULL;
        nameTag = "";
    }
    Thread(bool isDetached = false): runnable(NULL), detached(isDetached){
        _id = 0;
        arg = NULL;
        result = NULL;
        nameTag = "";
    } 
    virtual ~Thread() {}
    virtual void* Run() {}
    pthread_t GetId() { return _id; }
    string& GetNameTag() { return nameTag; }
    void SetNameTag(string& tag) { this->nameTag = tag; }
    void Start(void* arg=NULL);
    void* Join();
};


class Lock 
{ 
protected:
    pthread_mutex_t mutex;
    // Prevent copying or assignment
    Lock(const Lock& arg); 
    Lock& operator=(const Lock& lock);

public: 
    Lock();
    virtual ~Lock(); 
    void DoLock(); 
    void Unlock();
};


class Condition : public Lock 
{
protected:
    pthread_cond_t cond;
    // Prevent copying or assignment 
    Condition(const Condition& arg); 
    Condition& operator=(const Condition& co);

public: 
    Condition();
    virtual ~Condition(); 
    void Wait(); 
    void Notify();
};

class MessagePort;
class ThreadPortScheduler: public Thread
{
protected:
    string portName;
    EventMaster* eventMaster;
    MessagePort* msgPort;
    // Prevent copying or assignment 
    ThreadPortScheduler();
    ThreadPortScheduler& operator=(const ThreadPortScheduler& tp);
    
public:
    ThreadPortScheduler(string pn): portName(pn), eventMaster(NULL), msgPort(NULL) { }
    virtual ~ThreadPortScheduler();
    EventMaster* GetEventMaster() { return eventMaster; }
    MessagePort* GeMessagePort() { return msgPort; }
    virtual void* Run();
    virtual void* hookRun() {}
    virtual void hookHandleMessage() {}
};

#endif
