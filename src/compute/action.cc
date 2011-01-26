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
    switch (state)
    {
    case _Idle:
        state = _Started;

        try {
            //$$$$ may send out messages and add to expected topic list
            Process(); 

            //$$$$ schedule children

            //$$$$ call immediate children

        } catch (ComputeThreadException e) {
            LOG("Action::Run caught Exception:" << e.what() << " ErrMsg: " << e.GetMessage() << endl);
            state = _Failed;
            CleanUp();
        }

        if (expectMesssageTopics.size() == 0) 
        {
            if (childrenScheduled.size() == 0)
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
            }
        } catch (ComputeThreadException e) {
            LOG("Action::Run caught Exception:" << e.what() << " ErrMsg: " << e.GetMessage() << endl);
            state = _Failed;
            CleanUp();
        }

        // vvv fall through --> no break here by design vvv
    case _WaitingChildren:
        try {
            if (!ProcessChildren())
            {
                state = _WaitingChildren;
                Wait();
            }
        } catch (ComputeThreadException e) {
            LOG("Action::Run caught Exception:" << e.what() << " ErrMsg: " << e.GetMessage() << endl);
            state = _Failed;
            CleanUp();
        }
        
        //if all children are finished, we are done here.
        state = _Finished;
        Wait();
        break;

    case _Failed:
    case _Cancelled:
        CleanUp();
        break;

    case _Finished:
        CleanUp();
        Finish();
        break;
    default:
        //unknown state -->throw exception?
        ;
    }
}


void Action::Wait()
{
    repeats = FOREVER;
    worker->GetEventMaster()->Schedule(this);
}


void Action::Process()
{
    //$$$$ run current action main logic
    //$$$$ send out messages if needed
    //$$$$ change states if needed ?
}


//return true if all children finished otherwise false
bool Action::ProcessChildren()
{
    //$$$$ loop through all children to look for states
    //$$$$ return true if all children have finished
    //$$$$ otherwise false
}


//return true if all expected messages received otherwise false
bool Action::ProcessMessages()
{
    //$$$$ process messages if received
    //$$$$ run current action logic based on received messages 

    //$$$$ return true if all messages received and processed
    //$$$$ otherwise false
}



void Action::CleanUp()
{
    //$$$$ cancel all children actions

    //cleanning up data before being destroyed
    SetRepeats(0);
    SetObsolete(true);
    //?? doulbe check to remove from event loop?
}


void Action::Finish()
{
    //cleanning up data before being destroyed
    SetRepeats(0);
    SetObsolete(true);
}

