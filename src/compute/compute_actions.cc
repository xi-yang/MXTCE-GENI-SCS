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

#include "log.hh"
#include "event.hh"
#include "exception.hh"
#include "compute_actions.hh"


///////////////////// class Action_ProcessRequestTopology ///////////////////////////

void Action_ProcessRequestTopology::Process()
{
    LOG(name<<"Process() called"<<endl);
    //$$$$ run current action main logic

    //$$$$ send out messages if needed and add to expectMessageTopics
}


bool Action_ProcessRequestTopology::ProcessChildren()
{
    LOG(name<<"ProcessChildren() called"<<endl);
    //$$$$ loop through all children to look for states

    //$$$$ return true if all children have finished
    //$$$$ otherwise false
    return false;
}


bool Action_ProcessRequestTopology::ProcessMessages()
{
    LOG(name<<"ProcessMessages() called"<<endl);
    //$$$$ process messages if received
    //$$$$ run current action logic based on received messages 

    //$$$$ return true if all messages received and processed
    //$$$$ otherwise false
    return false;
}


void Action_ProcessRequestTopology::CleanUp()
{
    LOG(name<<"CleanUp() called"<<endl);
    Action::CleanUp();

    //$$$$ cleanup logic for current action
}


void Action_ProcessRequestTopology::Finish()
{
    LOG(name<<"Finish() called"<<endl);
    Action::Finish();

    //$$$$ finish logic for current action
}



///////////////////// class Action_CreateTEWG ///////////////////////////

void Action_CreateTEWG::Process()
{
    LOG(name<<"Process() called"<<endl);
    //$$$$ run current action main logic
    
    //$$$$ send out messages if needed and add to expectMessageTopics
}


bool Action_CreateTEWG::ProcessChildren()
{
    LOG(name<<"ProcessChildren() called"<<endl);
    //$$$$ loop through all children to look for states
    //$$$$ return true if all children have finished
    //$$$$ otherwise false
    return false;
}


bool Action_CreateTEWG::ProcessMessages()
{
    LOG(name<<"ProcessMessages() called"<<endl);
    //$$$$ process messages if received
    //$$$$ run current action logic based on received messages 

    //$$$$ return true if all messages received and processed
    //$$$$ otherwise false
    return false;
}


void Action_CreateTEWG::CleanUp()
{
    LOG(name<<"CleanUp() called"<<endl);
    Action::CleanUp();

    //$$$$ cleanup logic for current action
}


void Action_CreateTEWG::Finish()
{
    LOG(name<<"Finish() called"<<endl);
    Action::CleanUp();

    //$$$$ finish logic for current action
}



///////////////////// class Action_ComputeKSP ///////////////////////////

void Action_ComputeKSP::Process()
{
    LOG(name<<"Process() called"<<endl);
    //$$$$ run current action main logic
    //$$$$ send out messages if needed and add to expectMessageTopics

}


bool Action_ComputeKSP::ProcessChildren()
{
    LOG(name<<"ProcessChildren() called"<<endl);
    //$$$$ loop through all children to look for states
    //$$$$ return true if all children have finished
    //$$$$ otherwise false
    return false;
}


bool Action_ComputeKSP::ProcessMessages()
{
    LOG(name<<"ProcessMessages() called"<<endl);
    //$$$$ process messages if received
    //$$$$ run current action logic based on received messages 

    //$$$$ return true if all messages received and processed
    //$$$$ otherwise false
    return false;
}


void Action_ComputeKSP::CleanUp()
{
    LOG(name<<"CleanUp() called"<<endl);
    Action::CleanUp();

    //$$$$ cleanup logic for current action
}


void Action_ComputeKSP::Finish()
{
    LOG(name<<"Finish() called"<<endl);
    Action::CleanUp();

    //$$$$ finish logic for current action
}



///////////////////// class Action_FinalizeServiceTopology ///////////////////////////

void Action_FinalizeServiceTopology::Process()
{
    LOG(name<<"Process() called"<<endl);
    //$$$$ run current action main logic
    //$$$$ send out messages if needed and add to expectMessageTopics
}


bool Action_FinalizeServiceTopology::ProcessChildren()
{
    LOG(name<<"ProcessChildren() called"<<endl);
    //$$$$ loop through all children to look for states
    //$$$$ return true if all children have finished
    //$$$$ otherwise false
    return false;
}


bool Action_FinalizeServiceTopology::ProcessMessages()
{
    LOG(name<<"ProcessMessages() called"<<endl);
    //$$$$ process messages if received
    //$$$$ run current action logic based on received messages 

    //$$$$ return true if all messages received and processed
    //$$$$ otherwise false
    return false;
}


void Action_FinalizeServiceTopology::CleanUp()
{
    LOG(name<<"CleanUp() called"<<endl);
    Action::CleanUp();

    //$$$$ cleanup logic for current action
}


void Action_FinalizeServiceTopology::Finish()
{
    LOG(name<<"Finish() called"<<endl);
    Action::CleanUp();

    //$$$$ finish logic for current action
}


