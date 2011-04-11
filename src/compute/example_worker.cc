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
#include "example_worker.hh"

// Thread specific logic
void* ExampleComputeWorker::hookRun()
{
    // create workflow with action list (construct relationships)
    string actionName = "Example_Action_Process_RT";
    Action* actionRoot = new Action_ProcessRequestTopology(actionName, this);
    actions.push_back(actionRoot);

    actionName = "Example_Action_Create_TEWG";
    Action* actionNext = new Action_CreateTEWG(actionName, this);
    actions.push_back(actionNext);
    actionRoot->AddChild(actionNext);

    actionName = "Example_Action_Compute_KSP";
    Action* actionNext2 = new Action_ComputeKSP(actionName, this);
    actions.push_back(actionNext2);
    actionNext->AddChild(actionNext2);

    actionName = "Example_Action_Finalize_ST";
    Action* actionNext3 = new Action_FinalizeServiceTopology(actionName, this);
    actions.push_back(actionNext3);
    actionNext2->AddChild(actionNext3);
        
    // schedule the top level action(s)
    eventMaster->Schedule(actionRoot);
    //## eventMaster->Run() will be called by parent Run() 
}

// Handle message from thread message router
void ExampleComputeWorker::hookHandleMessage()
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

void ExampleComputeWorker::SetParameter(string& paramName, void* paramPtr)
{
    //if (paramName == "KSP")
    //    ksp = (vector<TPath*>*)paramPtr;
    //else 
        ComputeWorker::SetParameter(paramName, paramPtr);
}

void* ExampleComputeWorker::GetParameter(string& paramName)
{
    //if (paramName == "KSP")
    //    return ksp;
    //else 
        return ComputeWorker::GetParameter(paramName);
}

