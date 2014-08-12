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
 
#include "mxtce.hh"
#include "compute_worker.hh"
#include "simple_worker.hh"
#include "multip2p_worker.hh"
#include "mpvb_worker.hh"

list<ComputeWorker*> ComputeWorkerFactory::workers;
int ComputeWorkerFactory::serialNum = 0;
Lock ComputeWorkerFactory::cwfLock;

ComputeWorker::~ComputeWorker()
{
    ComputeWorkerFactory::RemoveComputeWorker(this->GetName());
}

Action* ComputeWorker::LookupAction(string& context, string& name)
{
    list<Action*>::iterator it = actions.begin();
    for (; it != actions.end(); it++)
    {
        if ((*it)->GetName() == name && (*it)->GetContext() == context)
            return (*it);
    }
    return NULL;
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
        this->HandleException(e);
        // TODO: Tell mxTCE core to detach the pipes ?
    }

    return pReturn;
}


void ComputeWorker::HandleException(ComputeThreadException& e)
{
    string gri = "";
    list<Apimsg_user_constraint*>* userConsList = (list<Apimsg_user_constraint*>*)this->GetWorkflowData("USER_CONSTRAINT_LIST");
    if (userConsList && userConsList->size() > 0)
    {
        Apimsg_user_constraint* userConstraint = userConsList->front();
        gri = userConstraint->getGri();
    }
    TLV* tlv = (TLV*)new char[TLV_HEAD_SIZE + sizeof(void*)];
    tlv->type = MSG_TLV_VOID_PTR;
    tlv->length = sizeof(void*);
    ComputeResult* result = new ComputeResult(gri);
    result->SetErrMessage(e.GetMessage());
    memcpy(tlv->value, &result, sizeof(void*));
    string queue = MxTCE::computeThreadPrefix + this->GetName();
    string topic = "COMPUTE_REPLY";
    Message* msg = new Message(MSG_REPLY, queue, topic);
    list<TLV*>::iterator itlv;
    msg->AddTLV(tlv);
    this->msgPort->GetWriter()->WriteMessage(msg);
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

        // loop through action list to match expectMessageTopics and deliver the message to action object
        list<Action*>::iterator ita;
        Action* action;
        for (ita = actions.begin(); ita != actions.end(); ita++) 
        {
            action = *ita;
            // context filtering
            if (!msg->GetContextTag().empty() && msg->GetContextTag() != action->GetContext())
                continue;
            // expected msg topic filtering
            list<string>::iterator its = action->GetExpectMessageTopics().begin();
            for (; its != action->GetExpectMessageTopics().end(); its++)
            {
                if ((*its) == msg->GetTopic())
                {
                    action->GetMessages().push_back(msg->Duplicate());
                    its = action->GetExpectMessageTopics().erase(its);
                }
            }
        }
        //delete msg in Action::ProcessMessages()
    }
}

//  data global for the workflow. Who sets the data will be responsible for cleanning them up.
void ComputeWorker::SetWorkflowData(const char* paramName, void* paramPtr)
{
    workflowData.SetData(paramName, paramPtr);
}

void ComputeWorker::SetWorkflowData(string& paramName, void* paramPtr)
{
    workflowData.SetData(paramName, paramPtr);
}

void* ComputeWorker::GetWorkflowData(const char* paramName)
{
    return workflowData.GetData(paramName);
}

void* ComputeWorker::GetWorkflowData(string& paramName)
{
    return workflowData.GetData(paramName);
}

// data for sub-level context (typically a path, connection or subset of the workflow) 
// stored in Action(s), who creates the data and will be responsible for cleanning them.
void* ComputeWorker::GetContextActionData(string& contextName, string& actionName, string& dataName)
{
    Action* action = LookupAction(contextName,actionName);
    if (action == NULL)
        return NULL;
    return action->GetData(dataName);
}

void* ComputeWorker::GetContextActionData(const char* contextName, const char* actionName, const char* dataName)
{
    string cn = contextName;
    string an = actionName;
    string dn = dataName;
    return GetContextActionData(cn, an, dn);
}

void* ComputeWorker::GetContextActionData(string& contextName, string& actionName, const char* dataName)
{
    string dn = dataName;
    return GetContextActionData(contextName, actionName, dn);
}

ComputeWorker* ComputeWorkerFactory::CreateComputeWorker(string type)
{
    ComputeWorkerFactory::cwfLock.DoLock();
    char buf[128];
    snprintf(buf, 128, "%s(%d)", type.c_str(), NewWorkerNum());

    ComputeWorker* worker;
    if (type =="simpleComputeWorker") 
        worker = new SimpleComputeWorker(buf);
    else if (type =="multip2pComputeWorker") 
        worker = new MultiP2PComputeWorker(buf);
    else if (type =="mpvbComputeWorker") 
        worker = new MPVBComputeWorker(buf);
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
    char str[1024];
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
    
