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

#include "compute_actions.hh"


///////////////////// class Action_ProcessRequestTopology ///////////////////////////

void Action_ProcessRequestTopology::Process()
{
    //$$$$ run current action main logic
    //$$$$ send out messages if needed

    //$$$$ change states into _waitingMesages  if messages are send out 
    //$$$$ TODO add messageSend function that keep records of ...
}


bool Action_ProcessRequestTopology::ProcessChildren()
{
    //$$$$ loop through all children to look for states
    //$$$$ return true if all children have finished
    //$$$$ otherwise false
}


bool Action_ProcessRequestTopology::ProcessMessages()
{
    //$$$$ process messages if received
    //$$$$ run current action logic based on received messages 

    //$$$$ return true if all messages received and processed
    //$$$$ otherwise false
}


void Action_ProcessRequestTopology::CleanUp()
{
    Action::CleanUp();

    //$$$$ cleanup logic for current action
}


void Action_ProcessRequestTopology::Finish()
{
    Action::CleanUp();

    //$$$$ finish logic for current action
}



///////////////////// class Action_CreateTEWG ///////////////////////////

void Action_CreateTEWG::Process()
{
    //$$$$ run current action main logic
    //$$$$ send out messages if needed

    //$$$$ change states into _waitingMesages  if messages are send out 
    //$$$$ TODO add messageSend function that keep records of ...
}


bool Action_CreateTEWG::ProcessChildren()
{
    //$$$$ loop through all children to look for states
    //$$$$ return true if all children have finished
    //$$$$ otherwise false
}


bool Action_CreateTEWG::ProcessMessages()
{
    //$$$$ process messages if received
    //$$$$ run current action logic based on received messages 

    //$$$$ return true if all messages received and processed
    //$$$$ otherwise false
}


void Action_CreateTEWG::CleanUp()
{
    Action::CleanUp();

    //$$$$ cleanup logic for current action
}


void Action_CreateTEWG::Finish()
{
    Action::CleanUp();

    //$$$$ finish logic for current action
}



///////////////////// class Action_ComputeKSP ///////////////////////////

void Action_ComputeKSP::Process()
{
    //$$$$ run current action main logic
    //$$$$ send out messages if needed

    //$$$$ change states into _waitingMesages  if messages are send out 
    //$$$$ TODO add messageSend function that keep records of ...
}


bool Action_ComputeKSP::ProcessChildren()
{
    //$$$$ loop through all children to look for states
    //$$$$ return true if all children have finished
    //$$$$ otherwise false
}


bool Action_ComputeKSP::ProcessMessages()
{
    //$$$$ process messages if received
    //$$$$ run current action logic based on received messages 

    //$$$$ return true if all messages received and processed
    //$$$$ otherwise false
}


void Action_ComputeKSP::CleanUp()
{
    Action::CleanUp();

    //$$$$ cleanup logic for current action
}


void Action_ComputeKSP::Finish()
{
    Action::CleanUp();

    //$$$$ finish logic for current action
}



///////////////////// class Action_FinalizeServiceTopology ///////////////////////////

void Action_FinalizeServiceTopology::Process()
{
    //$$$$ run current action main logic
    //$$$$ send out messages if needed

    //$$$$ change states into _waitingMesages  if messages are send out 
    //$$$$ TODO add messageSend function that keep records of ...
}


bool Action_FinalizeServiceTopology::ProcessChildren()
{
    //$$$$ loop through all children to look for states
    //$$$$ return true if all children have finished
    //$$$$ otherwise false
}


bool Action_FinalizeServiceTopology::ProcessMessages()
{
    //$$$$ process messages if received
    //$$$$ run current action logic based on received messages 

    //$$$$ return true if all messages received and processed
    //$$$$ otherwise false
}


void Action_FinalizeServiceTopology::CleanUp()
{
    Action::CleanUp();

    //$$$$ cleanup logic for current action
}


void Action_FinalizeServiceTopology::Finish()
{
    Action::CleanUp();

    //$$$$ finish logic for current action
}


