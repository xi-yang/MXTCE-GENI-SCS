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

#include "mpvb_worker.hh"
#include "compute_actions.hh"
#include "mpvb_actions.hh"
#include <cmath>


///////////////////// class Action_ProcessRequestTopology_MPVB ///////////////////////////

void Action_ProcessRequestTopology_MPVB::Process()
{
    // retrieve userConstrinat list
    list<Apimsg_user_constraint*>* userConsList = (list<Apimsg_user_constraint*>*)this->GetComputeWorker()->GetWorkflowData("USER_CONSTRAINT_LIST");
    if (userConsList == NULL)
    {
        throw ComputeThreadException((char*)"Action_ProcessRequestTopology_MPVB::Process() No USER_CONSTRAINT_LIST data from compute worker.");
    }
    Apimsg_user_constraint* userConstraint = userConsList->front();
    if (userConstraint->getMultiPointVlanMap() == NULL) 
    {
        throw ComputeThreadException((char*)"Action_ProcessRequestTopology_MPVB::Process() Recevied non-MVPB user constraint data from compute worker.");
    }    
    this->GetComputeWorker()->SetWorkflowData("USER_CONSTRAINT", userConstraint);

    // add Action_CreateTEWG
    string contextName = ""; // none
    string actionName = "Action_CreateTEWG";
    Action_CreateTEWG* actionTewg = new Action_CreateTEWG(contextName, actionName, this->GetComputeWorker());
    this->GetComputeWorker()->GetActions().push_back(actionTewg);
    this->AddChild(actionTewg);

    // add Action_PrestageCompute_MPVB
    actionName = "Action_PrestageCompute_MPVB";
    Action_PrestageCompute_MPVB* actionPrestage = new Action_PrestageCompute_MPVB(actionName, this->GetComputeWorker());
    this->GetComputeWorker()->GetActions().push_back(actionPrestage);
    actionTewg->AddChild(actionPrestage);

    // add Action_BridgeTerminal_MPVB
    actionName = "Action_BridgeTerminal_MPVB";
    Action_BridgeTerminal_MPVB* actionCompute = new Action_BridgeTerminal_MPVB(actionName, this->GetComputeWorker());
    this->GetComputeWorker()->GetActions().push_back(actionCompute);
    actionPrestage->AddChild(actionCompute);

    // add Action_FinalizeServiceTopology_MPVB
    actionName = "Action_FinalizeServiceTopology_MPVB";
    Action_FinalizeServiceTopology_MPVB* actionFinalize = new Action_FinalizeServiceTopology_MPVB(actionName, this->GetComputeWorker());
    this->GetComputeWorker()->GetActions().push_back(actionFinalize);
    actionCompute->AddChild(actionFinalize);
    
    // change to Timer event with a timeout value
    this->SetType(EVENT_TIMER);
    this->SetInterval(MPVB_COMPUTE_TIMEOUT, 0);
}

bool Action_ProcessRequestTopology_MPVB::ProcessChildren()
{
    LOG(name<<"ProcessChildren() called"<<endl);

    // check for timeout - throw exception if still having child working
    if (!Action::ProcessChildren())
   {
        // throwing exception will lead to Cleanup() that cancels all child actions
        throw ComputeThreadException((char*)"Action_ProcessRequestTopology_MPVB::ProcessChildren() Time out while computation is still in progress!");
    }
    return true;
}

bool Action_ProcessRequestTopology_MPVB::ProcessMessages()
{
    LOG(name<<"ProcessMessages() called"<<endl);
    return Action::ProcessMessages();
}

void Action_ProcessRequestTopology_MPVB::CleanUp()
{
    LOG(name<<"CleanUp() called"<<endl);
    Action::CleanUp();
}

void Action_ProcessRequestTopology_MPVB::Finish()
{

}


///////////////////// class Action_PrestageCompute_MPVB ///////////////////////////

void Action_PrestageCompute_MPVB::Process()
{
    Apimsg_user_constraint* userConstraint = (Apimsg_user_constraint*)this->GetComputeWorker()->GetWorkflowData("USER_CONSTRAINT");
    TEWG* tewg = (TEWG*)this->GetComputeWorker()->GetWorkflowData("TEWG");
    if (tewg == NULL)
    {
        throw ComputeThreadException((char*)"Action_ComputeKSP::Process() No TEWG available for computation!");
    }
    //set worker global parameters
    int D = (int)(sqrt((float)tewg->GetNodes().size())+1);
    int *pK = new int;
    *pK = (D > MAX_KSP_K ? MAX_KSP_K : D);
    this->GetComputeWorker()->SetWorkflowData("KSP_K", pK);
    int *pB = new int;
    *pB = BACKOFF_NUM;
    this->GetComputeWorker()->SetWorkflowData("BACKOFF_NUM", pB);
    int *pR = new int;
    *pR = MAX_REENTRY_NUM;
    this->GetComputeWorker()->SetWorkflowData("REENTRY_NUM", pR);
    
    // classify B,P,T nodes on TEWG
    list<TNode*> nodes = tewg->GetNodes();
    list<TNode*>::iterator itN = tewg->GetNodes().begin();
    list<TNode*>* terminals = new list<TNode*>;
    list<TNode*>* non_terminals = new list<TNode*>;
    for (; itN != tewg->GetNodes().end(); itN++) 
    {
        TNode* node = *itN;
        if (node->GetWorkData() == NULL)
            node->SetWorkData(new WorkData());
        map<string, string>::iterator itM = userConstraint->getMultiPointVlanMap()->begin();
        for (; itM != userConstraint->getMultiPointVlanMap()->end(); itM++)
        {
            if (node->VerifyContainUrn(itM->second))
            {
                int* pT = new int(MPVB_TYPE_T);
                node->GetWorkData()->SetData("MPVB_TYPE", pT);
                terminals->push_back(node); // add to terminal list
                break;
            }
        }
        if (itM != userConstraint->getMultiPointVlanMap()->end())
            continue;
        non_terminals->push_back(node);
        // check domain and node capabilities for "mp-l2-bridging" and mark as type B
        if (node->GetCapabilities().find("mp-l2-bridging") != node->GetCapabilities().end() 
            || (node->GetDomain()!= NULL && node->GetDomain()->GetCapabilities().find("mp-l2-bridging") != node->GetCapabilities().end()))
        {
            int* pB = new int(MPVB_TYPE_B);
            node->GetWorkData()->SetData("MPVB_TYPE", pB);
            continue;            
        }    
        // $$ mark all other nodes as type P (?)
        int* pP = new int(MPVB_TYPE_P);
        node->GetWorkData()->SetData("MPVB_TYPE", pP);
    }

    // create (re)ordered Terminal List {Z} and non-Terminal list {S}
    this->GetComputeWorker()->SetWorkflowData("ORDERED_TERMINALS", terminals); // Z
    this->GetComputeWorker()->SetWorkflowData("NON_TERMINALS", non_terminals); // S

    // TODO: reorder terminals {Z} by distance to graph mean / center?
    
    // worker-global pointer to current Terminal
    this->GetComputeWorker()->SetWorkflowData("CURRENT_TERMINAL", terminals->front());
    
    // create {T} graph 
    string tgName = "SMT";
    TGraph* SMT = new TGraph(tgName);
    for (itN = terminals->begin(); itN != terminals->end(); itN++) 
    {
	TDomain* td = (TDomain*)(*itN)->GetDomain();
	if (SMT->LookupDomainByName(td->GetName()) == NULL)
        {
             SMT->AddDomain(td);
	}
        SMT->AddNode(td, *itN);
    }
    this->GetComputeWorker()->SetWorkflowData("SERVICE_TOPOLOGY", SMT);

    // create KSP cache map (computed on the fly with cache search assistance)
    KSPCache* kspCache = new KSPCache();
    this->GetComputeWorker()->SetWorkflowData("KSP_CACHE", kspCache);
}

bool Action_PrestageCompute_MPVB::ProcessChildren()
{
    LOG(name<<"ProcessChildren() called"<<endl);

    // return true if all children have finished; otherwise false
    return Action::ProcessChildren();
}

bool Action_PrestageCompute_MPVB::ProcessMessages()
{
    LOG(name<<"ProcessMessages() called"<<endl);

    //return true if all messages received and processed; otherwise false
    return Action::ProcessMessages();
}

void Action_PrestageCompute_MPVB::CleanUp()
{
    LOG(name<<"CleanUp() called"<<endl);

    // cancel and cleanup children
    Action::CleanUp();
}

void Action_PrestageCompute_MPVB::Finish()
{

}


///////////////////// class Action_BridgeTerminal_MPVB ///////////////////////////

void Action_BridgeTerminal_MPVB::Process()
{
    TGraph* SMT = (TGraph*)this->GetComputeWorker()->GetWorkflowData("SERVICE_TOPOLOGY");
    list<TNode*>* orderedTerminals = (list<TNode*>*)this->GetComputeWorker()->GetWorkflowData("ORDERED_TERMINALS");
    list<TNode*>* nonTerminals = (list<TNode*>*)this->GetComputeWorker()->GetWorkflowData("NON_TERMINALS");
    TNode* currentTerminal = (TNode*)this->GetComputeWorker()->GetWorkflowData("CURRENT_TERMINAL");
    TNode* nextTerminal = NULL;

    list<TNode*> bridgeNodes;
    list<TNode*>::iterator itN;
    if (currentTerminal == orderedTerminals->front())
    {
        bridgeNodes.push_back(currentTerminal);
    }
    else
    {
        for (itN = SMT->GetNodes().begin(); itN != SMT->GetNodes().end(); itN ++)
        {
            TNode* node = *itN;
            if (node->GetWorkData()->GetInt("MPVB_TYPE") == MPVB_TYPE_B)
                bridgeNodes.push_back(node);
        }
    }
    if (bridgeNodes.empty()) 
    {
        throw ComputeThreadException((char*)"Action_BridgeTerminal_MPVB::Process Empty bridgeNodes list!");
    }
    for (itN = orderedTerminals->begin(); itN != orderedTerminals->end(); itN++) 
    {
        if ((*itN) == currentTerminal)
            break;
    }
    if (itN == orderedTerminals->end()) 
    {
        throw ComputeThreadException((char*)"Action_BridgeTerminal_MPVB::Process End of orderedTerminals list - no more terminal to process!");
    }
    // get the terminal node to be bridged
    nextTerminal = *itN;
    TNode* bridgeNode = NULL;
    TPath* bridgePath = NULL;
    for (itN = bridgeNodes.begin(); itN != bridgeNodes.end(); itN++) 
    {
        TNode* candidateNode = *itN;
        TPath* candidatePath = this->BridgeTerminalWithPDH(candidateNode, nextTerminal);
        if (candidatePath == NULL)
            continue;
        if (bridgePath == NULL)
        {
            bridgePath = candidatePath;
            bridgeNode = candidateNode;
        }
        else 
        {
            // compare two paths by cost - ? other criteria ? 
            if (bridgePath->GetCost() > candidatePath->GetCost())
            {
                bridgePath = candidatePath;
                bridgeNode = candidateNode;
            }
        }
        break;
    }
    
    int* pReentries = this->GetComputeWorker()->GetWorkflowData("REENTRY_NUM");
    if (bridgePath == NULL) // no feasible solution for bridging nextTerminal 
    {
        // crankback and retry with disturbing procedure
        // TODO: call back off (removal / reset logic) method
        // TODO: call reorder rest terminals method
        
        // Compute stop after timeout, but we still have a safe guard limit on number of reentries to make sure thread exit.
        int* pReentries = this->GetComputeWorker()->GetWorkflowData("REENTRY_NUM");
        --(*pReentries);
        if (*pReentries == 0)
        {
            throw ComputeThreadException((char*)"Action_BridgeTerminal_MPVB::Process Reach maximum number of reentries!");
        }
        this->GetComputeWorker()->SetWorkflowData("REENTRY_NUM", pReentries);
    }
    else // proceed to next terminal
    {        
        SMT->LoadPath(bridgePath); // will duplicate nodes/links cause trouble? (LoadPath should be indempotent) 
        currentTerminal = nextTerminal;
        if (currentTerminal != orderedTerminals->back()) 
        {        
            *pReentries = MAX_REENTRY_NUM;
            this->GetComputeWorker()->SetWorkflowData("REENTRY_NUM", pReentries);
        }
    }
    
    // add reentry / next Action_Compute_MPVB as child
    String actionName = "Action_BridgeTerminal_MPVB";
    Action_BridgeTerminal_MPVB* actionCompute = new Action_BridgeTerminal_MPVB(actionName, this->GetComputeWorker());
    this->GetComputeWorker()->GetActions().push_back(actionCompute);
    this->AddChild(actionCompute);
}

vector<TPath*>* Action_BridgeTerminal_MPVB::ComputeKSPWithCache(TNode* srcNode, TNode* dstNode)
{
    KSPCache* kspCache = (KSPCache*)this->GetComputeWorker()->GetWorkflowData("KSP_CACHE");
    vector<TPath*>* ksp = kspCache->Lookup(srcNode, dstNode);
    if (ksp != NULL) 
        return ksp;
    TEWG* tewg = (TEWG*)this->GetComputeWorker()->GetWorkflowData("TEWG");
    int* pK = (int*)this->GetComputeWorker()->GetWorkflowData("KSP_K");
    ksp = new vector<TPath*>;
    tewg->ComputeKShortestPaths(srcNode, dstNode, *pK, ksp);
    if (ksp.empty()) {
        delete ksp;
        return NULL;
    }
    kspCache->Add(srcNode, dstNode, ksp);
    return ksp;
}

// note: PDH = Path Distance Heuristic
TPath* Action_BridgeTerminal_MPVB::BridgeTerminalWithPDH(TNode* bridgeNode, TNode* terminalNode)
{
    TGraph* SMT = (TGraph*)this->GetComputeWorker()->GetWorkflowData("SERVICE_TOPOLOGY");
    vector<TPath*>* ksp = this->ComputeKSPWithCache(bridgeNode, terminalNode);
    if (ksp == NULL)
        return NULL;

    // TODO: verify SMT + paths
    // TODO: verify loopfree (candidatePath should not hit anothe node in SMT before the candidateNode)
    // TODO: pick and return a path (or NULL)
}

bool Action_BridgeTerminal_MPVB::ProcessChildren()
{
    LOG(name<<"ProcessChildren() called"<<endl);

    // return true if all children have finished; otherwise false
    return Action::ProcessChildren();
}

bool Action_BridgeTerminal_MPVB::ProcessMessages()
{
    LOG(name<<"ProcessMessages() called"<<endl);

    //return true if all messages received and processed; otherwise false
    return Action::ProcessMessages();
}

void Action_BridgeTerminal_MPVB::CleanUp()
{
    LOG(name<<"CleanUp() called"<<endl);

    // cancel and cleanup children
    Action::CleanUp();
}

void Action_BridgeTerminal_MPVB::Finish()
{
    // TODO: final improvement
}


///////////////////// class Action_FinalizeServiceTopology_MPVB ///////////////////////////

void Action_FinalizeServiceTopology_MPVB::Process()
{
    // process / transform successful result 
    // TODO: both success and failure replies are sent by Action_ProcessRequestTopology_MPVB::Finish
    
}

bool Action_FinalizeServiceTopology_MPVB::ProcessChildren()
{
    LOG(name<<"ProcessChildren() called"<<endl);

    // return true if all children have finished; otherwise false
    return Action::ProcessChildren();
}

bool Action_FinalizeServiceTopology_MPVB::ProcessMessages()
{
    LOG(name<<"ProcessMessages() called"<<endl);

    //return true if all messages received and processed; otherwise false
    return Action::ProcessMessages();
}

void Action_FinalizeServiceTopology_MPVB::CleanUp()
{
    LOG(name<<"CleanUp() called"<<endl);

    // cancel and cleanup children
    Action::CleanUp();
}

void Action_FinalizeServiceTopology_MPVB::Finish()
{

}


