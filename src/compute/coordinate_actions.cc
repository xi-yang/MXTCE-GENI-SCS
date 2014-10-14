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

#include "mxtce.hh"
#include "compute_worker.hh"
#include "compute_actions.hh"
#include "mpvb_actions.hh"
#include "coordinate_actions.hh"


///////////////////// class Action_ProcessRequestTopology_Coordinate ///////////////////////////

void Action_ProcessRequestTopology_Coordinate::Process()
{
    // retrieve userConstrinat list
    list<Apimsg_user_constraint*>* userConsList = (list<Apimsg_user_constraint*>*)this->GetComputeWorker()->GetWorkflowData("USER_CONSTRAINT_LIST");
    if (userConsList == NULL || userConsList->empty())
    {
        throw ComputeThreadException((char*)"Action_ProcessRequestTopology_Coordinate::Process() No USER_CONSTRAINT_LIST data from compute worker.");
    }

    // add Action_CreateTEWG
    string contextName = ""; // none
    string actionName = "Action_CreateTEWG";
    Action_CreateTEWG* actionTewg = new Action_CreateTEWG(contextName, actionName, this->GetComputeWorker());
    this->GetComputeWorker()->GetActions().push_back(actionTewg);
    this->AddChild(actionTewg);

    //$$ add Action_CheckResult_Coordinate (in ::Finish method)
    actionName = "Action_CheckResult_Coordinate";
    Action_CheckResult_Coordinate* actionCheckResult = new Action_CheckResult_Coordinate(contextName, actionName, this->GetComputeWorker());
    this->GetComputeWorker()->GetActions().push_back(actionCheckResult);
    actionTewg->AddChild(actionCheckResult);

    //$$ add Action_ProcessSubworker_Coordinate (multiple as children of Action_CheckResult_Coordinate, each with separate contextName)
    list<Apimsg_user_constraint*> mp2pUserConsList;
    list<Apimsg_user_constraint*>::iterator itUC = userConsList->begin();
    for (; itUC != userConsList->end(); itUC++)
    {
        Apimsg_user_constraint* UC = *itUC;
        if (UC->getMultiPointVlanMap() == NULL)
            mp2pUserConsList.push_back(UC);
        else 
        {
            contextName = this->worker->GetName() + UC->getPathId();
            actionName = "Action_ProcessSubworker_Coordinate";
            Action_ProcessSubworker_Coordinate* actionProcSubworker = new Action_ProcessSubworker_Coordinate(contextName, actionName, this->GetComputeWorker());
            this->GetComputeWorker()->GetActions().push_back(actionProcSubworker);
            actionCheckResult->AddChild(actionProcSubworker);
            // add userConstraint to ContextActionData of the subworker action
            list<Apimsg_user_constraint*>* mpvbUserConsList = (list<Apimsg_user_constraint*>*)this->worker->GetContextActionData(contextName, actionName, "USER_CONSTRAINT_LIST");
            mpvbUserConsList->push_back(UC);
        }
    }
    if (!mp2pUserConsList.empty())
    {
        contextName = this->worker->GetName() + "-MP2P";
        actionName = "Action_ProcessSubworker_Coordinate";
        Action_ProcessSubworker_Coordinate* actionProcSubworker = new Action_ProcessSubworker_Coordinate(contextName, actionName, this->GetComputeWorker());
        this->GetComputeWorker()->GetActions().push_back(actionProcSubworker);
        actionCheckResult->AddChild(actionProcSubworker);
        // add userConstraint to ContextActionData of the subworker action
        list<Apimsg_user_constraint*>* userConsList = (list<Apimsg_user_constraint*>*)this->worker->GetContextActionData(contextName, actionName, "USER_CONSTRAINT_LIST");
        userConsList->assign(mp2pUserConsList.begin(), mp2pUserConsList.end());
    }
}

bool Action_ProcessRequestTopology_Coordinate::ProcessChildren()
{
    LOG(name<<"ProcessChildren() called"<<endl);

    return Action::ProcessChildren();
}

bool Action_ProcessRequestTopology_Coordinate::ProcessMessages()
{
    LOG(name<<"ProcessMessages() called"<<endl);
    return Action::ProcessMessages();
}

void Action_ProcessRequestTopology_Coordinate::CleanUp()
{
    LOG(name<<"CleanUp() called"<<endl);
    Action::CleanUp();
}

void Action_ProcessRequestTopology_Coordinate::Finish()
{
    list<Apimsg_user_constraint*>* userConsList = (list<Apimsg_user_constraint*>*)this->GetComputeWorker()->GetWorkflowData("USER_CONSTRAINT_LIST");
    string* errMsg = (string*)this->GetComputeWorker()->GetWorkflowData("ERROR_MSG");
    list<TLV*> tlvList;
    if (errMsg != NULL && !errMsg->empty())
    {
        ComputeResult* result = new ComputeResult(userConsList->front()->getGri());
        result->SetErrMessage(*errMsg);
        TLV* tlv = (TLV*)new char[TLV_HEAD_SIZE + sizeof(void*)];
        tlv->type = MSG_TLV_VOID_PTR;
        tlv->length = sizeof(void*);
        memcpy(tlv->value, &result, sizeof(void*));
        tlvList.push_back(tlv);
    }
    else 
    {
        // create coordinated compute results by combining grandchildren (Action_ProcessSubworker_Coordinate)
        list<Action*>::iterator itA = this->worker->GetActions().begin();
        for (; itA != this->children.end(); itA++)
        {
            Action* action = *itA;
            if (action->GetName().compare("Action_ProcessSubworker_Coordinate") != 0)
                continue;
            Action_ProcessSubworker_Coordinate* subworkerAction = (Action_ProcessSubworker_Coordinate*)*itA;
            list<ComputeResult*>* computeResultList = (list<ComputeResult*>*)this->worker->GetContextActionData(subworkerAction->GetContext(), subworkerAction->GetName(), "COMPUTE_RESULT_LIST");
            list<ComputeResult*>::iterator itR = computeResultList->begin();
            for (; itR != computeResultList->end(); itR++)
            {
                ComputeResult* result = *itR;
                TLV* tlv = (TLV*)new char[TLV_HEAD_SIZE + sizeof(void*)];
                tlv->type = MSG_TLV_VOID_PTR;
                tlv->length = sizeof(void*);
                memcpy(tlv->value, &result, sizeof(void*));
                tlvList.push_back(tlv);
            }
        }
    }
    string queue = MxTCE::computeThreadPrefix + this->GetComputeWorker()->GetName();
    string topic = "COMPUTE_REPLY";
    SendMessage(MSG_REPLY, queue, topic, tlvList);

    // stop out from event loop
    Action::Finish();
}


///////////////////// class Action_CheckResult_Coordinate ///////////////////////////

void Action_CheckResult_Coordinate::Process()
{
    // no-op
}

bool Action_CheckResult_Coordinate::ProcessChildren()
{
    LOG(name<<"ProcessChildren() called"<<endl);

    //$$ check for timeout - throw exception if still having child working

    // If any subworker has failed, fail naively ajd cancel other processing subworkers
    bool hasFailure = false;
    list<Action*>::iterator itA = this->children.begin();
    for (; itA != this->children.end(); itA++)
    {
        Action* action = *itA;
        if (action->GetName().compare("Action_ProcessSubworker_Coordinate") == 0 && action->GetState() == _Failed)
        {
            hasFailure = true;
            break;
        }
    }
    if (hasFailure)
    {
        for (itA = this->children.begin(); itA != this->children.end(); itA++)
        {
            Action* action = *itA;
            if (action->GetName().compare("Action_ProcessSubworker_Coordinate") == 0
                && (action->GetState() != _Failed || action->GetState() != _Finished || action->GetState() != _Cancelled))
            {
                action->SetState(_Cancelled);
            }
        }
        return false;
    }
    else
        return Action::ProcessChildren();
}

bool Action_CheckResult_Coordinate::ProcessMessages()
{
    LOG(name<<"ProcessMessages() called"<<endl);
    return Action::ProcessMessages();
}

void Action_CheckResult_Coordinate::CleanUp()
{
    LOG(name<<"CleanUp() called"<<endl);
    Action::CleanUp();
}

void Action_CheckResult_Coordinate::Finish()
{
    // Every subworker has finished or failed.
    // Examine children / subworkers and retrieve results

    // If any of the subworkers has failed, fail naively
        // TODO: Future improvement will retain the succesful ones and retry the failed ones by reconditioning the requests.
    list<Action*>::iterator itA = this->children.begin();
    for (; itA != this->children.end(); itA++)
    {
        Action* action = *itA;
        if (action->GetState() == _Failed && action->GetName().compare("Action_ProcessSubworker_Coordinate") == 0)
        {
            string dataName = "COMPUTE_RESULT_LIST";
            list<ComputeResult*>* computeResultList = (list<ComputeResult*>*)((Action_ProcessSubworker_Coordinate*)action)->GetData(dataName);
            ComputeResult* errResult = computeResultList->front();
            char buf[256];
            snprintf(buf, 255, "Action_CheckResult_Coordinate::Finish() Subworker %s failed with errMsg [%s]\n", action->GetContext().c_str(), errResult->GetErrMessage().c_str());
            LOGF(buf);
            throw ComputeThreadException(buf);
        }
    }
    //$$$$ if all suscessful, check conflcts 
        // If any conflict, fail naively, no attempt to resolve the conflict for now.
        // TODO: Future improvement will try resolve simple conflicts and  even retry the offending subworkers by reconditioning the requests.

    // stop out from event loop
    Action::Finish();
}


///////////////////// class Action_ProcessSubworker_Coordinate ///////////////////////////

// Action_ProcessSubworker_Coordinate keeps request and reply data locally as ContextActionData 
void Action_ProcessSubworker_Coordinate::_Init()
{
    this->_userConstraintList = new list<Apimsg_user_constraint*>;
    this->_computeResultList = new list<ComputeResult*>;
}

void Action_ProcessSubworker_Coordinate::Process()
{
    // create subworker computing thread
    ComputeWorker* computingThread;
    if (this->_userConstraintList->front()->getMultiPointVlanMap() == NULL)
        computingThread = ComputeWorkerFactory::CreateComputeWorker("multip2pComputeWorker");
    else
        computingThread = ComputeWorkerFactory::CreateComputeWorker("mpvbComputeWorker");
    
    if (computingThread == NULL) 
    {
        LOGF("Action_ProcessSubworker_Coordinate::Process() Cannot create subworker thread of type!");
        throw ComputeThreadException((char*)"Action_ProcessSubworker_Coordinate::Process() Cannot create subworker thread of type!");
    }

    computingThread->SetWorkflowData("USER_CONSTRAINT_LIST", this->_userConstraintList);
    computingThread->SetWorkflowData("COMPUTE_CONTEXT", &this->context);

    // create message routing entries
    string computeThreadPortName = computingThread->GetName();
    MxTCE* mxTCE = (MxTCE*)this->worker->GetWorkflowData("MXTCE_CORE");
    mxTCE->GetMessageRouter()->AddPort(computeThreadPortName);
    string computeThreadQueueName = MxTCE::computeThreadPrefix + computingThread->GetName();
    string routeTopic1 = "COMPUTE_REQUEST", routeTopic2 = "COMPUTE_REPLY",
        routeTopic3 = "TEWG_REQUEST", routeTopic4 = "TEWG_RESV_REQUEST", routeTopic5 = "TEWG_REPLY", 
        routeTopic6 = "POLICY_REQUEST", routeTopic7 = "POLICY_REPLY";
    // coodinatorThread (this) send compute request to computeThread
    mxTCE->GetMessageRouter()->AddRoute(computeThreadQueueName, routeTopic1, computeThreadPortName);
    // computeThread reply to coodinatorThread (this)  with computation result
    mxTCE->GetMessageRouter()->AddRoute(computeThreadQueueName, routeTopic2, this->worker->GetMessagePort()->GetName());
    // tedbMan request
    mxTCE->GetMessageRouter()->AddRoute(computeThreadQueueName, routeTopic3, MxTCE::tedbManPortName);
    // resvMan request
    mxTCE->GetMessageRouter()->AddRoute(computeThreadQueueName, routeTopic4, MxTCE::resvManPortName);
    // resvMan reply to coodinatorThread (this) 
    mxTCE->GetMessageRouter()->AddRoute(computeThreadQueueName, routeTopic5, computeThreadPortName);
    // policyMan request
    mxTCE->GetMessageRouter()->AddRoute(computeThreadQueueName, routeTopic6, MxTCE::policyManPortName);
    // policyMan reply to coodinatorThread (this) 
    mxTCE->GetMessageRouter()->AddRoute(computeThreadQueueName, routeTopic7, computeThreadPortName);
    
    // start subworker computing thread
    computingThread->Start(NULL);

    // create and send request message to subworker computing thread
    string queue = computeThreadQueueName;
    string topic = "COMPUTE_REQUEST";
    string expectReturnTopic = "COMPUTE_REPLY";
    list<TLV*> TLVs;
    list<Apimsg_user_constraint*>::iterator itUC = this->_userConstraintList->begin();
    for (; itUC != this->_userConstraintList->end(); itUC++)
    {
        Apimsg_user_constraint* userCons = * itUC;
        TLV* tlv = (TLV*)(new u_int8_t[TLV_HEAD_SIZE + sizeof(userCons)]);
        tlv->type = MSG_TLV_VOID_PTR;
        tlv->length = sizeof(userCons);
        memcpy(tlv->value, &userCons, sizeof(userCons));
        TLVs.push_back(tlv);
    }
    SendMessage(MSG_REQ, queue, topic, this->context, TLVs, expectReturnTopic);
}

bool Action_ProcessSubworker_Coordinate::ProcessChildren()
{
    LOG(name<<"ProcessChildren() called"<<endl);

    return Action::ProcessChildren();
}

bool Action_ProcessSubworker_Coordinate::ProcessMessages()
{
    LOG(name<<"ProcessMessages() called"<<endl);

    // run current action logic based on received messages 
    list<Message*>::iterator itm;
    Message* msg;
    for (itm = messages.begin(); itm != messages.end(); itm++)
    {
        msg = *itm;
        if (msg->GetTopic().compare("COMPUTE_REPLY") == 0)
        {
            ComputeResult* result = NULL;
            list<TLV*>& tlvList = msg->GetTLVList();
            list<TLV*>::iterator itLV = tlvList.begin();
            for (; itLV != tlvList.end(); itLV++)
            {
                memcpy(&result, (*itLV)->value, sizeof(result));
                this->_computeResultList->push_back(result);
                if (!result->GetErrMessage().empty())
                {
                    throw ComputeThreadException(result->GetErrMessage());
                }
            }
        }
        //delete msg; //msg consumed 
        itm = messages.erase(itm);
    }

    return Action::ProcessMessages();
}

void Action_ProcessSubworker_Coordinate::CleanUp()
{
    LOG(name<<"CleanUp() called"<<endl);

    //$$ delete locally kept request / result data
    
    // Do not delete: the lists are used by other components
    //delete this->_userConstraintList;
    //delete this->_computeResultList;
    
    Action::CleanUp();
}

void Action_ProcessSubworker_Coordinate::Finish()
{   
    // stop out from event loop
    Action::Finish();
}

void* Action_ProcessSubworker_Coordinate::GetData(string& dataName)
{
    if (dataName.compare("USER_CONSTRAINT_LIST") == 0)
        return this->_userConstraintList;
    else if (dataName.compare("COMPUTE_RESULT_LIST") == 0)
        return this->_computeResultList;
    return NULL;
}

