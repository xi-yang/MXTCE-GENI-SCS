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

#include "compute_worker.hh"
#include "simple_worker.hh"

list<ComputeWorker*> ComputeWorkerFactory::workers;
int ComputeWorkerFactory::serialNum = 0;
Lock ComputeWorkerFactory::cwfLock;

ComputeWorker::~ComputeWorker()
{
    ComputeWorkerFactory::RemoveComputeWorker(this->GetName());
}

void* ComputeWorker::Run()
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
            LOG("ComputeWorker::Run AttachPipes on client side" << endl);
        } catch (MsgIOException& e) {
            LOG("ComputeWorker::Run caugh Exception: " << e.what() << endl);
        }
    }

    // call job function
    pReturn = this->hookRun();

    // start event loop
    try {
        eventMaster->Run();
    } catch (ComputeThreadException e) {
        string paramName = "ERRPR_MSG";
        string& errMsg = e.GetMessage();
        SetParameter(paramName,&errMsg);
        // TODO: Tell mxTCE core to detach the pipes ?
    }

    return pReturn;
}


// Thread specific logic
void* ComputeWorker::hookRun()
{
    //$$$  specific workflow init

    // eventMaster->Run() will be initiated in parent class ThreadPortScheduler::Run() method
}


// Handle message from thread message router
void ComputeWorker::hookHandleMessage()
{
    Message* msg = NULL;
    while ((msg = msgPort->GetMessage()) != NULL)
    {
        msg->LogDump();
        //delete msg; //msg consumed
    }
}


void ComputeWorker::SetParameter(string& paramName, void* paramPtr)
{
    if (paramName == "TEWG")
        tewg = (TEWG*)paramPtr;
    else if (paramName == "USER_CONSTRAINT")
        userConstraint = (Apimsg_user_constraint*)paramPtr;
    else if (paramName == "ERROR_MSG")
        errMsg = *(string*)paramPtr;
    else if (paramName == "KSP")
        ksp = (vector<TPath*>*)paramPtr;
    else if (paramName == "FEASIBLE_PATHS")
        feasiblePaths= (vector<TPath*>*)paramPtr;
}

void* ComputeWorker::GetParameter(string& paramName)
{
    if (paramName == "TEWG")
        return tewg;    
    else if (paramName == "USER_CONSTRAINT")
        return userConstraint;
    else if (paramName == "ERROR_MSG")
        return &errMsg;
    else if (paramName == "KSP")
        return ksp;
    else if (paramName == "FEASIBLE_PATHS")
        return feasiblePaths;
    return NULL;
}


ComputeWorker* ComputeWorkerFactory::CreateComputeWorker(string type)
{
    ComputeWorkerFactory::cwfLock.DoLock();
    char buf[128];
    snprintf(buf, 128, "%s(%d)", type.c_str(), NewWorkerNum());

    ComputeWorker* worker;
    if (type =="simpleComputeWorker") 
        worker = new SimpleComputeWorker(buf);
    else 
    {   
        snprintf(buf, 128, "Unknown computeWorkerThread type: %s", type.c_str());
        ComputeWorkerFactory::cwfLock.Unlock();
        throw TCEException(buf);
    }
    workers.push_back(worker);
    ComputeWorkerFactory::cwfLock.Unlock();
    return worker;
}


ComputeWorker* ComputeWorkerFactory::LookupComputeWorker(string name)
{
    ComputeWorkerFactory::cwfLock.DoLock();
    list<ComputeWorker*>::iterator it;
    for (it = workers.begin(); it != workers.end(); it++)
    {
        if ((*it)->GetName() == name)
        {
            ComputeWorkerFactory::cwfLock.Unlock();
            return (*it);
        }
    }
    ComputeWorkerFactory::cwfLock.Unlock();
    return NULL;
}


void ComputeWorkerFactory::RemoveComputeWorker(string name)
{
    ComputeWorkerFactory::cwfLock.DoLock();
    list<ComputeWorker*>::iterator it;
    for (it = workers.begin(); it != workers.end(); it++)
    {
        if ((*it)->GetName() == name)
        {
            workers.erase(it);
            ComputeWorkerFactory::cwfLock.Unlock();
            return;
       }
    }
    ComputeWorkerFactory::cwfLock.Unlock();
}


void ComputeResult::RegulatePathInfo(TPath* path)
{
    if (path == NULL)
        return;
    // set link name to full qualified URN
    TLink* L;
    string urn;
    char str[128];
    list<TLink*>::iterator itL = path->GetPath().begin();
    for (; itL != path->GetPath().end(); itL++)
    {
        L = *itL;
        if (strstr(L->GetName().c_str(), "urn:ogf:network") != NULL)
            continue;
        sprintf(str, "urn:ogf:network:domain=%s:node=%s:port=%s:link=%s",
            L->GetPort()->GetNode()->GetDomain()->GetName().c_str(),
            L->GetPort()->GetNode()->GetName().c_str(),
            L->GetPort()->GetName().c_str(),
            L->GetName().c_str());
        urn = str;
        L->SetName(urn);
    }
    path->GetPath().front()->SetRemoteLink(NULL);
    path->GetPath().back()->SetRemoteLink(NULL);
}
    
