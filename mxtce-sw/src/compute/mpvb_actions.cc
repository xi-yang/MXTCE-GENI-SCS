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
    //this->SetType(EVENT_TIMER);
    //this->SetInterval(MPVB_COMPUTE_TIMEOUT, 0);
}

bool Action_ProcessRequestTopology_MPVB::ProcessChildren()
{
    LOG(name<<"ProcessChildren() called"<<endl);

    // check for timeout - throw exception if still having child working
    /*
    if (!Action::ProcessChildren())
    {
        // throwing exception will lead to Cleanup() that cancels all child actions
        throw ComputeThreadException((char*)"Action_ProcessRequestTopology_MPVB::ProcessChildren() Time out while computation is still in progress!");
    }
    return true;
    */
    return Action::ProcessChildren();
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
    Apimsg_user_constraint* userConstraint = (Apimsg_user_constraint*)this->GetComputeWorker()->GetWorkflowData("USER_CONSTRAINT");

    list<TLV*> tlvList;
    string* errMsg = (string*)this->GetComputeWorker()->GetWorkflowData("ERROR_MSG");
    ComputeResult* result = new ComputeResult(userConstraint->getGri());
    result->SetPathId(userConstraint->getPathId());
    if (errMsg != NULL && !errMsg->empty())
    {
        result->SetErrMessage(*errMsg);
    }
    else 
    {
        // TODO: assemble MPVB TGraph into ComputeResult
        TGraph* SMT = (TGraph*)this->GetComputeWorker()->GetWorkflowData("SERVICE_TOPOLOGY");
        result->SetGraphInfo(SMT);
    }
    TLV* tlv = (TLV*)new char[TLV_HEAD_SIZE + sizeof(void*)];
    tlv->type = MSG_TLV_VOID_PTR;
    tlv->length = sizeof(void*);
    memcpy(tlv->value, &result, sizeof(void*));
    tlvList.push_back(tlv);
    string queue = MxTCE::computeThreadPrefix + this->GetComputeWorker()->GetName();
    string topic = "COMPUTE_REPLY";
    SendMessage(MSG_REPLY, queue, topic, tlvList);

    // stop out from event loop
    Action::Finish();
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
    //prune low bandwidth links
    tewg->PruneByBandwidth(userConstraint->getBandwidth());
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
    vector<TNode*>* terminals = new vector<TNode*>;
    vector<TNode*>* non_terminals = new vector<TNode*>;
    for (; itN != tewg->GetNodes().end(); itN++) 
    {
        TNode* node = *itN;
        if (node->GetWorkData() == NULL)
            node->SetWorkData(new TWorkData());
        map<string, string>::iterator itM = userConstraint->getMultiPointVlanMap()->begin();
        for (; itM != userConstraint->getMultiPointVlanMap()->end(); itM++)
        {
            if (node->VerifyContainUrn((string&)itM->first))
            {
                int* pT = new int(MPVB_TYPE_T);
                node->GetWorkData()->SetData("MPVB_TYPE", pT);
                terminals->push_back(node); // add to terminal vector
                string* pV = new string(itM->second);
                node->GetWorkData()->SetData("VLAN_RANGE", pV);
                break;
            }
        }
        if (itM != userConstraint->getMultiPointVlanMap()->end())
            continue;
        non_terminals->push_back(node);
        // check domain and node capabilities for "mp-l2-bridging" and mark as type B
        if (node->GetCapabilities().find("mp-l2-bridging") != node->GetCapabilities().end() 
            || (node->GetDomain()!= NULL && node->GetDomain()->GetCapabilities().find("mp-l2-bridging") != node->GetDomain()->GetCapabilities().end()))
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
    
    // create {T} graph  
    // note: all elements (domains/ nodes/ ports /links) must be duplicates from TEWG
    string tgName = "SMT";
    TGraph* SMT = new TGraph(tgName);
    this->GetComputeWorker()->SetWorkflowData("SERVICE_TOPOLOGY", SMT);
    /*
    vector<TNode*>::iterator itvN;
    for (itvN = terminals->begin(); itvN != terminals->end(); itvN++) 
    {
	TDomain* td = ((TDomain*)(*itvN)->GetDomain())->Clone();
	if (SMT->LookupDomainByName(td->GetName()) == NULL)
        {
             SMT->AddDomain(td);
	}
        SMT->AddNode(td, (*itvN)->Clone());
    }
    */

    this->SeedBridgeWithLPH();
    
    terminals = (vector<TNode*>*)this->GetComputeWorker()->GetWorkflowData("ORDERED_TERMINALS");
    this->GetComputeWorker()->SetWorkflowData("CURRENT_TERMINAL", (*terminals)[1]);

    // create KSP cache map (computed on the fly with cache search assistance)
    KSPCache* kspCache = new KSPCache();
    this->GetComputeWorker()->SetWorkflowData("KSP_CACHE", kspCache);
}

// note: LPH = Longest Path Heuristic
void Action_PrestageCompute_MPVB::SeedBridgeWithLPH()
{
    TGraph* SMT = (TGraph*)this->GetComputeWorker()->GetWorkflowData("SERVICE_TOPOLOGY");
    TEWG* tewg = (TEWG*)this->GetComputeWorker()->GetWorkflowData("TEWG");
    vector<TNode*>* terminals = (vector<TNode*>*)this->GetComputeWorker()->GetWorkflowData("ORDERED_TERMINALS");
    
    int i, j;
    for (i = 0; i < terminals->size(); i++) 
    {
        TNode* srcNode = (*terminals)[i];
        for (j = 0; j < terminals->size(); j++)
        {
            if (j == i)
                continue;
            TNode* dstNode = (*terminals)[j];
            dstNode->SetId(j);
            try {
                list<TLink*> path = tewg->ComputeDijkstraPath(srcNode, dstNode, true, true);
                long* pSD = (long*)srcNode->GetWorkData()->GetData("PRE_SUM_DISTANCE");
                if (pSD == NULL)
                {
                    pSD = new long(0);
                }
                *pSD += path.size();
                srcNode->GetWorkData()->SetData("PRE_SUM_DISTANCE", pSD);

                long* pMD = (long*)srcNode->GetWorkData()->GetData("PRE_MAX_DISTANCE");
                u_int32_t* pMNID = (u_int32_t*)srcNode->GetWorkData()->GetData("PRE_MAX_NODE_ID");
                if (pMD == NULL)
                {
                    pMD = new long(0);
                    pMNID = new u_int32_t(0);
                }
                if (*pMD < path.size()) {
                    *pMD = path.size();
                    *pMNID = dstNode->GetId();
                }
                srcNode->GetWorkData()->SetData("PRE_MAX_DISTANCE", pMD);
                srcNode->GetWorkData()->SetData("PRE_MAX_NODE_ID", pMNID);
            } 
            catch (TCEException ex)
            {
                throw ComputeThreadException((char*)"Action_PrestageCompute_MPVB::Process TEWG is disconnected between some terminals!");
            }
        }
    }
    
    // find two terminals with the longest path in between and add them as first and second 
    vector<TNode*>* reorderedTerminals = new vector<TNode*>;
    int first = 0;
    for (i = 1; i < terminals->size(); i++) 
    {
        if ((*terminals)[first]->GetWorkData()->GetLong("PRE_MAX_DISTANCE") < (*terminals)[i]->GetWorkData()->GetLong("PRE_MAX_DISTANCE"))
            first = i;
    }
    reorderedTerminals->push_back((*terminals)[first]);
    int second = (*terminals)[first]->GetWorkData()->GetInt("PRE_MAX_NODE_ID");
    reorderedTerminals->push_back((*terminals)[second]);
    terminals->erase(terminals->begin()+first);
    terminals->erase(terminals->begin()+second - (second>first?1:0));
    // reorder remaining terminals by raw sum distance to others (ascending)
    while (!terminals->empty())
    {
        long min = 0;
        long sum = _INF_;
        for (i = 0; i < terminals->size(); i++)
        {
            if (sum > (*terminals)[i]->GetWorkData()->GetLong("PRE_SUM_DISTANCE"))
            {
                sum = (*terminals)[i]->GetWorkData()->GetLong("PRE_SUM_DISTANCE");
                min = i;
            }
        }
        reorderedTerminals->push_back((*terminals)[min]);
        terminals->erase(terminals->begin()+min);
    }

    this->GetComputeWorker()->SetWorkflowData("ORDERED_TERMINALS", reorderedTerminals);
    
    // add the longest path (seed path) to SMT
    int* pK = (int*)this->GetComputeWorker()->GetWorkflowData("KSP_K");
    vector<TPath*> KSP;
    tewg->ComputeKShortestPaths((*reorderedTerminals)[0], (*reorderedTerminals)[1], *pK, KSP);
    if (KSP.empty()) {
        throw ComputeThreadException((char*)"Action_PrestageCompute_MPVB::Process Cannot find feasible seeding path!");
    }
    bool found = false;
    for (i = 0; i < KSP.size(); i++)
    {
        TPath* P = KSP[i];
        //verify path TE / VLAN constraints
        string srcVlan = *(string*)(*reorderedTerminals)[0]->GetWorkData()->GetData("VLAN_RANGE");
        string dstVlan = *(string*)(*reorderedTerminals)[1]->GetWorkData()->GetData("VLAN_RANGE");
        TServiceSpec srcTSS(LINK_IFSWCAP_L2SC, LINK_IFSWCAP_ENC_ETH, 1, srcVlan);
        TServiceSpec dstTSS(LINK_IFSWCAP_L2SC, LINK_IFSWCAP_ENC_ETH, 1, dstVlan);
        if (P->VerifyTEConstraints(srcTSS, dstTSS, true) && !found) 
        {
            // $$ use the path with max(dstTSS.GetVlanSet().size()) instead ?
            SMT->LoadPath(P->GetPath());
            TNode* terminalA = SMT->LookupSameNode((*reorderedTerminals)[0]);
            TNode* terminalZ = SMT->LookupSameNode((*reorderedTerminals)[1]);
            if (terminalA->GetWorkData() == NULL)
                terminalA->SetWorkData(new WorkData);
            terminalA->GetWorkData()->SetData("VLAN_RANGE", new string(srcVlan));
            if (terminalZ->GetWorkData() == NULL)
                terminalZ->SetWorkData(new WorkData);
            terminalZ->GetWorkData()->SetData("VLAN_RANGE", new string(srcVlan));
            LOG_DEBUG("SeedLongestPath:");
	        //P->LogDump();
            found = true;
        }
        delete P;
    }
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
    TEWG* tewg = (TEWG*)this->GetComputeWorker()->GetWorkflowData("TEWG");
    vector<TNode*>* orderedTerminals = (vector<TNode*>*)this->GetComputeWorker()->GetWorkflowData("ORDERED_TERMINALS");
    vector<TNode*>* nonTerminals = (vector<TNode*>*)this->GetComputeWorker()->GetWorkflowData("NON_TERMINALS");
    TNode* currentTerminal = (TNode*)this->GetComputeWorker()->GetWorkflowData("CURRENT_TERMINAL");
    TNode* nextTerminal = NULL;

    list<TNode*> bridgeNodes;
    list<TNode*>::iterator itN;
    for (itN = SMT->GetNodes().begin(); itN != SMT->GetNodes().end(); itN ++)
    {
        TNode* node = tewg->LookupSameNode(*itN); // replace with same node in TEWG
        // skip artificial ("*" containing) nodes
        if (node->GetWorkData()->GetInt("MPVB_TYPE") == MPVB_TYPE_B && node->GetName().find("*") == string::npos)
            bridgeNodes.push_back(node);
    }
    if (bridgeNodes.empty()) 
    {
        throw ComputeThreadException((char*)"Action_BridgeTerminal_MPVB::Process Empty bridgeNodes list!");
    }
    vector<TNode*>::iterator itvN = orderedTerminals->begin();
    for (; itvN != orderedTerminals->end(); itvN++) 
    {
        if ((*itvN) == currentTerminal)
            break;
    }
    itvN++;
    if (itvN == orderedTerminals->end()) 
    {
        throw ComputeThreadException((char*)"Action_BridgeTerminal_MPVB::Process End of orderedTerminals list - no more terminal to process!");
    }
    // get the terminal node to be bridged
    nextTerminal = *itvN;
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
            // compare two paths by cost  (+ other criteria ? )
            if (bridgePath->GetCost() > candidatePath->GetCost())
            {
                bridgePath = candidatePath;
                bridgeNode = candidateNode;
            }
        }
        
        // ? keep/cache other qualified paths for disturbing procedure?
    }
    
    int* pReentries = (int*)this->GetComputeWorker()->GetWorkflowData("REENTRY_NUM");
    if (bridgePath == NULL) // no feasible solution for bridging nextTerminal 
    {
        // crankback and retry with disturbing procedure
        // call back off (removal / reset logic) and disturb (reorder)  method ( go back up to 2nd node in the list)
        this->BackoffAndDisturb();
        
        // Compute stop after timeout, but we still have a safe guard limit on number of reentries to make sure thread exit.
        --(*pReentries);
        if (*pReentries == 0)
        {
            throw ComputeThreadException((char*)"Action_BridgeTerminal_MPVB::Process Reach maximum number of reentries!");
        }
    }
    else // proceed to next terminal
    {
        SMT->LoadPath(bridgePath->GetPath()); //  (LoadPath should be indempotent at domain and node levels, but not for port and link) 
        TNode* terminalZ = SMT->LookupSameNode(nextTerminal);
        if (terminalZ->GetWorkData() == NULL)
            terminalZ->SetWorkData(new WorkData);
        terminalZ->GetWorkData()->SetData("VLAN_RANGE", new string(*(string*)nextTerminal->GetWorkData()->GetData("VLAN_RANGE")));
        this->GetComputeWorker()->SetWorkflowData("CURRENT_TERMINAL", nextTerminal);
        *pReentries = MAX_REENTRY_NUM;
        if (nextTerminal == orderedTerminals->back()) 
            return; // we have bridged all terminals sucessfully
    }
    // add reentry / next Action_BridgeTerminal_MPVB as child
    this->GetComputeWorker()->SetWorkflowData("REENTRY_NUM", pReentries);
    string actionName = "Action_BridgeTerminal_MPVB";
    Action_BridgeTerminal_MPVB* actionCompute = new Action_BridgeTerminal_MPVB(actionName, this->GetComputeWorker());
    this->GetComputeWorker()->GetActions().push_back(actionCompute);
    actionCompute->GetChildren().assign(this->GetChildren().begin(), this->GetChildren().end());
    this->GetChildren().clear();
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
    tewg->ComputeKShortestPaths(srcNode, dstNode, *pK, *ksp);
    if (ksp->empty()) {
        delete ksp;
        return NULL;
    }
    kspCache->Add(srcNode, dstNode, ksp);
    return ksp;
}

// note: PDH = Path Distance Heuristic
TPath* Action_BridgeTerminal_MPVB::BridgeTerminalWithPDH(TNode* bridgeNode, TNode* terminalNode)
{
    TEWG* tewg = (TEWG*)this->GetComputeWorker()->GetWorkflowData("TEWG");
    terminalNode = tewg->LookupSameNode(terminalNode); // replace terminalNode  with same node in TEWG
    bridgeNode = tewg->LookupSameNode(bridgeNode); // replace bridgeNode  with same node in TEWG
    vector<TPath*>* ksp = this->ComputeKSPWithCache(bridgeNode, terminalNode);
    if (ksp == NULL)
        return NULL;

    TPath* bridgePath = NULL;
    vector<TPath*>::iterator itP = ksp->begin();
    for (; itP != ksp->end(); itP++)
    {
        if (this->VerifyBridgePath(bridgeNode, terminalNode, *itP))
        {
            bridgePath = *itP;
            break;
        }
        
        // ? keep/cache other qualified paths for disturbing procedure?
    }
    return bridgePath;
}

bool Action_BridgeTerminal_MPVB::VerifyBridgePath(TNode* bridgeNode, TNode* terminalNode, TPath* bridgePath)
{
    TGraph* SMT = (TGraph*)this->GetComputeWorker()->GetWorkflowData("SERVICE_TOPOLOGY");
    TEWG* tewg = (TEWG*)this->GetComputeWorker()->GetWorkflowData("TEWG");
    bridgeNode = SMT->LookupSameNode(bridgeNode);   //replace bridgeNode  with same node in SMT

    //LOG_DEBUG("VerifyBridgePath:");
    bridgePath->LogDump();
    //1. verify loopfree: candidatePath should not hit anothe node in SMT besides the bridgeNode
    list<TLink*>& pathLinks = bridgePath->GetPath();
    list<TLink*>::iterator itL = pathLinks.begin();
    // note: direction bridge -> terminal
    for (; itL != pathLinks.end(); itL++)
    {
        TNode* nextNode = (*itL)->GetRemoteEnd();
        if (nextNode == NULL || nextNode->GetName().find("*") != string::npos)
            continue;
        nextNode = SMT->LookupSameNode(nextNode);
        if (nextNode != NULL && nextNode != bridgeNode)
            return false;
    }

    //2. verify TE constraint for SMT+bridgePath
    terminalNode = tewg->LookupSameNode(terminalNode); // replace terminalNode  with same node in TEWG
    string terminalVlan = terminalNode->GetWorkData()->GetString("VLAN_RANGE");
    TServiceSpec terminalTspec(LINK_IFSWCAP_L2SC, LINK_IFSWCAP_ENC_ETH, 1, terminalVlan);
    string bridgeVlan = "any";
    TServiceSpec bridgeTspec(LINK_IFSWCAP_L2SC, LINK_IFSWCAP_ENC_ETH, 1, bridgeVlan);
    if (!bridgePath->VerifyTEConstraints(terminalTspec,bridgeTspec, true))
        return false;
    if (!SMT->VerifyMPVBConstraints(bridgeNode, bridgeTspec.GetVlanSet()))
        return false;
}


void Action_BridgeTerminal_MPVB::BackoffAndDisturb()
{
    TGraph* SMT = (TGraph*)this->GetComputeWorker()->GetWorkflowData("SERVICE_TOPOLOGY");
    TEWG* tewg = (TEWG*)this->GetComputeWorker()->GetWorkflowData("TEWG");
    vector<TNode*>* orderedTerminals = (vector<TNode*>*)this->GetComputeWorker()->GetWorkflowData("ORDERED_TERMINALS");
    TNode* currentTerminal = (TNode*)this->GetComputeWorker()->GetWorkflowData("CURRENT_TERMINAL");
    int backoffNum = *(int*)this->GetComputeWorker()->GetWorkflowData("BACKOFF_NUM");

    int i = 0;
    for (; i < orderedTerminals->size(); i++) 
    {
        if (orderedTerminals->at(i) == currentTerminal)
            break;
    }
    if (i < backoffNum) // no backoff, just reorder [i+1:]
    {
        backoffNum = 0;
    }
    else if (i == backoffNum) // backoff 1
    {
        backoffNum = 1;
    }
    if ((i-backoffNum+1) == orderedTerminals->size()-1)
    {
        // nothing left to disturb
        throw ComputeThreadException((char*)"Action_BridgeTerminal_MPVB::BackoffAndDisturb No feasible solution - without room to further disturb the bridging order!");
    }

    int j = 0;
    for (; j < backoffNum; j++)
    {
        TNode* backoffTerminal = orderedTerminals->at(i-j);
        backoffTerminal = SMT->LookupSameNode(backoffTerminal); // replace with 
        // starting from the terminal remove   connected the links and nodes from SMT until reaching a bridgeNode (node with links)
        this->BackoffFromTerminal(SMT, backoffTerminal);
    }

    // reorder terminals after (*orderedTerminals)[i-backoff] - simply move next terminal to end of list (?)
    orderedTerminals->push_back(orderedTerminals->at(i+1));
    orderedTerminals->erase(orderedTerminals->begin()+i+1);
}

void Action_BridgeTerminal_MPVB::BackoffFromTerminal(TGraph* SMT, TNode* terminal)
{
    if (terminal->GetLocalLinks().size() != 1)
        return;
    TLink* nextLink = terminal->GetLocalLinks().front();
    TLink* nextRemoteLink = (TLink*)nextLink->GetRemoteLink();
    TNode* nextNode = nextLink->GetRemoteEnd();
    SMT->RemoveLink(nextLink);
    if (nextRemoteLink)
        SMT->RemoveLink(nextRemoteLink);
    SMT->RemoveNode(terminal);
    this->BackoffFromTerminal(SMT, nextNode);
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
}


///////////////////// class Action_FinalizeServiceTopology_MPVB ///////////////////////////

void Action_FinalizeServiceTopology_MPVB::Process()
{
    // ?? SMT-PDH final improvement ?

    // finalize VLAN bridging for SMT nodes and links 
    TGraph* SMT = (TGraph*)this->GetComputeWorker()->GetWorkflowData("SERVICE_TOPOLOGY");
    vector<TNode*>* orderedTerminals = (vector<TNode*>*)this->GetComputeWorker()->GetWorkflowData("ORDERED_TERMINALS");
    TNode* firstTerminal = orderedTerminals->at(0);
    string terminalVlan = firstTerminal->GetWorkData()->GetString("VLAN_RANGE");
    firstTerminal = SMT->LookupSameNode(firstTerminal); // replace terminal with SMT node
    TServiceSpec terminalTspec(LINK_IFSWCAP_L2SC, LINK_IFSWCAP_ENC_ETH, 1, terminalVlan);
    if (!SMT->VerifyMPVBConstraints(firstTerminal, terminalTspec.GetVlanSet(), true)) // finalizeVlan = true
        throw ComputeThreadException((char*)"Action_FinalizeServiceTopology_MPVB::Process Failed to verify final SMT VLANs!");
    // modify graph bandwidth to service bandwidth
    Apimsg_user_constraint* userConstraint = (Apimsg_user_constraint*)this->GetComputeWorker()->GetWorkflowData("USER_CONSTRAINT");
    u_int64_t bw = userConstraint->getBandwidth();
    list<TLink*>::iterator itL = SMT->GetLinks().begin();
    for (; itL != SMT->GetLinks().end(); itL++)
        (*itL)->SetMaxBandwidth(bw);
    SMT->LogDump();
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


