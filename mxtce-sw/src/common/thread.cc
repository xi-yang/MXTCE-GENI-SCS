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

#include "exception.hh"
#include "thread.hh"


//Class Thread

// thread start function when a Runnable is involved
void* Thread::ExecRunnable(void* pVoid)
{   Thread* runnableThread = static_cast<Thread*> (pVoid); 
    assert(runnableThread); 
    runnableThread->result = runnableThread->runnable->Run();
    return runnableThread->result;
}


// thread start function when no Runnable is involved 
void* Thread::Exec(void* pVoid) 
{ 
    Thread* aThread = static_cast<Thread*> (pVoid); 
    assert(aThread); 
    aThread->result = aThread->Run(); 
    return aThread->result;
}


void* Thread::Join() 
{ 
    int status = pthread_join(_id,NULL); 
    // result was already saved by thread start functions 
    if (status != 0) 
    { 
        throw(new ThreadException("Thread:Join(" + this->GetNameTag() + ") failed on pthread_join"));
    }
    return arg;
}


void Thread::Start(void* arg) 
{ 
    this->arg = arg;
    int status = pthread_attr_init(&threadAttribute); 
    // initialize attribute object
    if (status != 0) 
    { 
        throw ThreadException("Thread::Start(" + this->GetNameTag() + ") failed on pthread_attr_init");
    }
    status = pthread_attr_setscope(&threadAttribute, PTHREAD_SCOPE_SYSTEM); 
    if (status != 0) 
    { 
        throw ThreadException("Thread::Start(" + this->GetNameTag() + ") failed on pthread_attr_setscope");
    }
    if (!detached) 
    {
        if (runnable.get() == NULL)
            status = pthread_create(&_id,&threadAttribute, Thread::Exec,(void*) this); 
        else 
             status = pthread_create(&_id,&threadAttribute, Thread::ExecRunnable, (void*)this);
        if (status != 0)
        {
            throw ThreadException("Thread::Start(" + this->GetNameTag() + ") failed on pthread_create");
        }
    }
    else
    {
        // set the detachstate attribute to detached 
        status = pthread_attr_setdetachstate(&threadAttribute, PTHREAD_CREATE_DETACHED); 
        if (status != 0)
        {
           throw ThreadException("Thread::Start(" + this->GetNameTag() + ") failed on pthread_attr_setdetachstate");
        } 
        if (runnable.get() == NULL)
            status = pthread_create(&_id,&threadAttribute, Thread::Exec, (void*) this); 
        else
            status = pthread_create(&_id,&threadAttribute, Thread::ExecRunnable, (void*) this); 
        if (status != 0) 
        {
            throw ThreadException("Thread::Start(" + this->GetNameTag() + ") failed on pthread_create (detached)");
        }
    }
    status = pthread_attr_destroy(&threadAttribute); 
    if (status != 0) 
    {
        throw ThreadException("Thread::Start(" + this->GetNameTag() + ") failed on pthread_attr_destroy");
    }
}



//Class Lock

Lock::Lock() 
{ 
    pthread_mutex_init(&mutex, NULL);
}

Lock::~Lock() 
{ 
    pthread_mutex_destroy(&mutex);
}


void Lock::DoLock() 
{
    pthread_mutex_lock(&mutex); 
}


void Lock::Unlock() 
{    
    pthread_mutex_unlock(&mutex);
}


// Class Condition

Condition::Condition() 
{ 
    pthread_cond_init(&cond, NULL);
}


Condition::~Condition() 
{
    pthread_cond_destroy(&cond);
}


void Condition::Wait() 
{ 
    pthread_cond_wait(&cond, &mutex);
}


void Condition::Notify() 
{
    pthread_cond_signal(&cond);
}



// Class ThreadPortScheduler

ThreadPortScheduler::~ThreadPortScheduler()
{
    msgPort->DetachPipes();
}


void* ThreadPortScheduler::Run()
{
    // init event manster
    if (eventMaster == NULL)
        eventMaster = new EventMaster;

    msgPort = MessagePipeFactory::LookupMessagePipe(portName)->GetClientPort();
    assert(msgPort);
    msgPort->SetEventMaster(eventMaster);
    msgPort->SetThreadScheduler(this);

    void* pReturn = NULL;
    if (!msgPort->IsUp())
    {
        try {
            msgPort->AttachPipes();
        } catch (MsgIOException& e) {
            LOG("ThreadPortScheduler::Run caugh Exception: " << e.what() << endl);
        }
    }

    // call job function
    pReturn = this->hookRun();

    // start event loop
    eventMaster->Run();
    return pReturn;
}

