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
        break;
    case _PendingMsg:
        Wait();
        break;
    case _ReceivedMsg:
        Process();
        break;
    case _WaitingChildren:
        //check children states
        //wait() unless fisnied
        break;
    case _Cancelled:
        //?
        break;
    case _Failed:
        //?
        break;
    case _Finished:
        //?
        break;
    default:
        //unknown state -->throw exception?
        ;
    }
}


void Action::Process()
{
    //process messages 
    //change states if needed, then --> Run()
}


void Action::ProcessChildren()
{
    //loop through all children and schedule them
    //if there is at least one
        // Wait()
    //otherwise
        //Finish()
}


void Action::Wait()
{
    repeats = FOREVER;
    worker->GetEventMaster()->Schedule(this);
}


void Action::WaitForChildren()
{
    //check wether all childrenScheduled have finished (or timed out?)
    //repeats = FOREVER;
    //worker->GetEventMaster()->Schedule(this);
}

void Action::Finish()
{
    state = _Finished;
    repeats = 0;
}

void Action::CleanUp()
{
    //cleanning up before being destroyed
    //state = _Finished;
    //repeats = 0;
}

