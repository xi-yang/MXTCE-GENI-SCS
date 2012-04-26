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

#include "event.hh"
#include "log.hh"
#include "exception.hh"
#include "message.hh"
#include "action.hh"
#include "compute_worker.hh"


void Action::Run()
{
    list<Action*>::iterator ita;
    char buf[256];
    string errMsg;

    if (!worker->GetMessagePort()->IsUp())
    {
        worker->GetEventMaster()->Remove(this);
        this->SetRepeats(0);
        state = _Failed; 
        CleanUp();
        // ???
        throw ComputeThreadException((char*)" Action::Run abort due to messagePort down.");
    }

    switch (state)
    {
    case _Idle:
        state = _Started;

        try {
            //immediate execution of current action logic
            Process(); 
        } catch (ComputeThreadException e) {
            state = _Failed;
            CleanUp();
            snprintf(buf, 256, "Action::Run caught Exception: %s", e.what());
            LOG(buf << endl);
            if (parent == NULL) // root action will fail directly as no parent will clean up for it
                throw ComputeThreadException(buf);
            errMsg = buf;
            this->GetComputeWorker()->SetWorkflowData("ERROR_MSG", &errMsg);
            break;
        }

        if (expectMesssageTopics.size() == 0) 
        {
            if (children.size() == 0)
            {
                state = _Finished;
                Finish();
                break;
            }
            else
            {
                //schedule children actions
                for (ita = children.begin(); ita != children.end(); ita++)
                    worker->GetEventMaster()->Schedule(*ita);
                state = _WaitingChildren;
            }
        }
        else 
        {
            state = _WaitingMessages;
        }

        Wait();
        break;

    case _WaitingMessages:
        try {
            if (!ProcessMessages())
            {
                state = _WaitingMessages;
                Wait();
                break;
            }
        } catch (ComputeThreadException e) {
            state = _Failed;
            CleanUp();
            snprintf(buf, 256, "Action::Run caught Exception: %s", e.what());
            LOG(buf << endl);
            if (parent == NULL) // root action will fail directly as no parent will clean up for it
                throw ComputeThreadException(buf);
            errMsg = buf;
            this->GetComputeWorker()->SetWorkflowData("ERROR_MSG", &errMsg);
            break;
        }

        //schedule children actions
        for (ita = children.begin(); ita != children.end(); ita++)
            worker->GetEventMaster()->Schedule(*ita);

        //## vvv Fall through to next case --> no break here by design vvv
    case _WaitingChildren:
        try {
            if (!ProcessChildren())
            {
                state = _WaitingChildren;
                Wait();
                break;
            }
        } catch (ComputeThreadException e) {
            state = _Failed;
            CleanUp();
            snprintf(buf, 256, "Action::Run caught Exception: %s", e.what());
            LOG(buf << endl);
            if (parent == NULL) // root action will fail directly as no parent will clean up for it
                throw ComputeThreadException(buf);
            errMsg = buf;
            this->GetComputeWorker()->SetWorkflowData("ERROR_MSG", &errMsg);
            break;
        }
        
        //if all children are finished, we are done with the current action.
        state = _Finished;

        //## vvv Fall through to next case --> no break here by design vvv
    case _Finished:
        CleanUp();
        Finish();
        break;

    case _Cancelled:
    case _Failed:
        CleanUp();
        break;

    default:
        snprintf(buf, 256, "Action::Run() gets into unknown state: %d\n", state);
        LOG(buf << endl);
        CleanUp();
        throw ComputeThreadException(buf);
    }
}

 
void Action::Wait()
{
    repeats = FOREVER;
    worker->GetEventMaster()->Remove(this);
    worker->GetEventMaster()->Schedule(this);
}


void Action::SendMessage(MessageType type, string& queue, string& topic, list<TLV*>& tlvs)
{
    Message* msg = new Message(type, queue, topic);
    list<TLV*>::iterator itlv;
    for (itlv = tlvs.begin(); itlv != tlvs.end(); itlv++)
        msg->AddTLV(*itlv);
    GetComputeWorker()->GetMessagePort()->PostMessage(msg);
}

void Action::SendMessage(MessageType type, string& queue, string& topic, string& contextTag, list<TLV*>& tlvs)
{
    Message* msg = new Message(type, queue, topic);
    msg->SetContextTag(contextTag);
    list<TLV*>::iterator itlv;
    for (itlv = tlvs.begin(); itlv != tlvs.end(); itlv++)
        msg->AddTLV(*itlv);
    GetComputeWorker()->GetMessagePort()->PostMessage(msg);
}

void Action::SendMessage(MessageType type, string& queue, string& topic, list<TLV*>& tlvs, string& expectReturnTopic)
{
    SendMessage(type, queue, topic, tlvs);
    expectMesssageTopics.push_back(expectReturnTopic);
}

void Action::SendMessage(MessageType type, string& queue, string& topic, string& contextTag, list<TLV*>& tlvs, string& expectReturnTopic)
{
    SendMessage(type, queue, topic, contextTag, tlvs);
    expectMesssageTopics.push_back(expectReturnTopic);
}

void Action::Process()
{
    // do nothing in root Action class
    ;  
    // in derived classes, run current action main logic
    // send out messages if needed and add to expectMessageTopics
}


//return true if all children finished otherwise false
bool Action::ProcessChildren()
{
    // loop through all children to look for states
    list<Action*>::iterator ita;
    Action* action;
    for (ita = children.begin(); ita != children.end(); ita++)
    {
        action = *ita;
        if (action->GetState() != _Finished && action->GetState() != _Cancelled && action->GetState() != _Failed)
        {
            return false;
        }
    }

    return true;
}


//return true if all expected messages received otherwise false
bool Action::ProcessMessages()
{
    return (expectMesssageTopics.size() == 0);
}



void Action::CleanUp()
{
    // loop through all children to cleanup
    list<Action*>::iterator ita;
    Action* action;
    for (ita = children.begin(); ita != children.end(); ita++)
    {
        action = *ita;
        if (action->GetState() != _Finished && action->GetState() != _Failed)
        {
            action->SetState(_Cancelled);
            action->CleanUp();
        }
        if (action->GetState() == _Failed)
            this->SetState(_Failed);
    }

    //cleanning up data before being destroyed
    SetRepeats(0);
    SetObsolete(true);
}


void Action::Finish()
{
    //delete data before being destroyed
    worker->GetEventMaster()->Remove(this);
}

