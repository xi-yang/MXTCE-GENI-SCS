/*
 * Copyright (c) 2013-2014
 * GENI Project.
 * University of Maryland /Mid-Atlantic Crossroads (UMD/MAX).
 * All rights reserved.
 *
 * Created by Xi Yang 2014
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
#include "mpvb_actions.hh"


void Action_ProcessRequestTopology_MPVB::Process()
{
    //
}


bool Action_ProcessRequestTopology_MPVB::ProcessChildren()
{
    LOG(name<<"ProcessChildren() called"<<endl);
    //$$$$ loop through all children to look for states

    // return true if all children have finished; otherwise false
    return Action::ProcessChildren();
}


bool Action_ProcessRequestTopology_MPVB::ProcessMessages()
{
    LOG(name<<"ProcessMessages() called"<<endl);
    //$$$$ process messages if received
    //$$$$ run current action logic based on received messages 

    //return true if all messages received and processed; otherwise false
    return Action::ProcessMessages();
}


void Action_ProcessRequestTopology_MPVB::CleanUp()
{
    LOG(name<<"CleanUp() called"<<endl);
    //$$$$ cleanup logic for current action

    // cancel and cleanup children
    Action::CleanUp();
}


void Action_ProcessRequestTopology_MPVB::Finish()
{

}

