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
    std::stringstream ssMsg;

    switch (state)
    {
    case _Idle:
        state = _Started;

        try {
            //immediate execution of current action logic
            Process(); 
            //schedule children actions
            list<Action*>::iterator ita;
            for (ita = children.begin(); ita != children.end(); ita++)
                worker->GetEventMaster()->Schedule(*ita);
        } catch (ComputeThreadException e) {
            LOG("Action::Run caught Exception:" << e.what() << " ErrMsg: " << e.GetMessage() << endl);
            state = _Failed;
            CleanUp();
        }

        if (expectMesssageTopics.size() == 0) 
        {
            if (children.size() == 0)
                state = _Finished;
            else
                state = _WaitingChildren;
        }
        else 
        {
            state = _WaitingMessages;
        }

        Wait();
        break;

    case _WaitingMessages:
        try {
            if (!ProcessMessages() || expectMesssageTopics.size()>0) // true -> all messages received, other wise false
            {
                state = _WaitingMessages;
                Wait();
                break;
            }
        } catch (ComputeThreadException e) {
            LOG("Action::Run caught Exception:" << e.what() << " ErrMsg: " << e.GetMessage() << endl);
            state = _Failed;
            CleanUp();
        }

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
            LOG("Action::Run caught Exception:" << e.what() << " ErrMsg: " << e.GetMessage() << endl);
            state = _Failed;
            CleanUp();
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
        ssMsg << "Action::Run() gets into unknown state: "<<state;
        LOG(ssMsg.str()<<endl);
        CleanUp();
        throw ComputeThreadException(ssMsg.str());
    }
}

 
void Action::Wait()
{
    repeats = FOREVER;
    worker->GetEventMaster()->Remove(this);
    worker->GetEventMaster()->Schedule(this);
    this->SetNice(true); //make sure messages can be sent and received during wait
}


void Action::SendMessage(MessageType type, string& queue, string& topic, list<TLV*>& tlvs)
{
    Message* msg = new Message(type, queue, topic);
    list<TLV*>::iterator itlv;
    for (itlv = tlvs.begin(); itlv != tlvs.end(); itlv++)
        msg->AddTLV(*itlv);
    GetComputeWorker()->GeMessagePort()->PostMessage(msg);
}


void Action::SendMessage(MessageType type, string& queue, string& topic, list<TLV*>& tlvs, string& expectReturnTopic)
{
    SendMessage(type, queue, topic, tlvs);
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

