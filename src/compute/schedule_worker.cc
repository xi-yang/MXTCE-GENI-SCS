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
#include "schedule_worker.hh"

// Thread specific logic
void* ScheduleComputeWorker::hookRun()
{
    // create workflow with action list (construct relationships)
    string actionName = "Simple_Action_Process_RT";
    Action* actionRoot = new Action_ProcessRequestTopology(actionName, this);
    actions.push_back(actionRoot);

    actionName = "Simple_Action_Create_TEWG";
    Action* actionNext = new Action_CreateTEWG(actionName, this);
    actions.push_back(actionNext);
    actionRoot->AddChild(actionNext);

    actionName = "Simple_Action_Compute_KSP";
    Action* actionNext2 = new Action_ComputeKSP(actionName, this);
    actions.push_back(actionNext2);
    actionNext->AddChild(actionNext2);

    actionName = "Simple_Action_Create_OrderedATS";
    Action* actionNext3 = new Action_CreateOrderedATS(actionName, this);
    actions.push_back(actionNext3);
    actionNext2->AddChild(actionNext3);

    actionName = "Simple_Action_Compute_Schedules";
    Action* actionNext4 = new Action_ComputeSchedulesWithKSP(actionName, this);
    actions.push_back(actionNext4);
    actionNext3->AddChild(actionNext4);

    actionName = "Simple_Action_Finalize_ST";
    Action* actionNext5 = new Action_FinalizeServiceTopology(actionName, this);
    actions.push_back(actionNext5);
    actionNext4->AddChild(actionNext5);

    // schedule the top level action(s)
    eventMaster->Schedule(actionRoot);
    //## eventMaster->Run() will be called by parent Run() 
}

