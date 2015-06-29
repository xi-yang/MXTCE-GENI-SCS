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
#include "mxtce.hh"
#include "tewg.hh"
#include "scheduling.hh"
#include "compute_worker.hh"
#include "compute_actions.hh"
#include <algorithm> 


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

    // return true if all children have finished; otherwise false
    return Action::ProcessChildren();
}


bool Action_ProcessRequestTopology::ProcessMessages()
{
    LOG(name<<"ProcessMessages() called"<<endl);
    //$$$$ process messages if received
    //$$$$ run current action logic based on received messages 

    //return true if all messages received and processed; otherwise false
    return Action::ProcessMessages();
}


void Action_ProcessRequestTopology::CleanUp()
{
    LOG(name<<"CleanUp() called"<<endl);
    //$$$$ cleanup logic for current action

    // cancel and cleanup children
    Action::CleanUp();
}


void Action_ProcessRequestTopology::Finish()
{
    LOG(name<<"Finish() called"<<endl);

    list<Apimsg_user_constraint*>* userConsList = (list<Apimsg_user_constraint*>*)this->GetComputeWorker()->GetWorkflowData("USER_CONSTRAINT_LIST");
    if (userConsList == NULL || userConsList->size() == 0)
        throw ComputeThreadException((char*)"Action_ProcessRequestTopology_MP2P::Process() No USER_CONSTRAINT_LIST data from compute worker.");

    Apimsg_user_constraint* userConstraint = userConsList->front();
    ComputeResult* result = new ComputeResult(userConstraint->getGri());

    vector<TPath*>* feasiblePaths = (vector<TPath*>*)this->GetComputeWorker()->GetWorkflowData("FEASIBLE_PATHS");
    if (feasiblePaths && feasiblePaths->size() > 0)
    {
        TPath* resultPath = feasiblePaths->front()->Clone();
        resultPath->LogDump();
        resultPath->SetIndependent(true); 
        ComputeResult::RegulatePathInfo(resultPath);
        result->SetPathInfo(resultPath);
        if (userConstraint->getCoschedreq()&& feasiblePaths->size() > 1) 
        {
            result->GetAlterPaths().clear();
            for (int k = 1; k < feasiblePaths->size() && k < userConstraint->getCoschedreq()->getMaxnumofaltpaths(); k++)
            {
                resultPath = (*feasiblePaths)[k]->Clone();
                resultPath->LogDump();
                resultPath->SetIndependent(true); 
                ComputeResult::RegulatePathInfo(resultPath);
                result->GetAlterPaths().push_back(resultPath);
            }
        }
    }
    else
    {
        result->SetErrMessage(*(string*)(this->GetComputeWorker()->GetWorkflowData("ERROR_MSG")));
    }
    TLV* tlv = (TLV*)new char[TLV_HEAD_SIZE + sizeof(void*)];
    tlv->type = MSG_TLV_VOID_PTR;
    tlv->length = sizeof(void*);
    memcpy(tlv->value, &result, sizeof(void*));
    string queue = MxTCE::computeThreadPrefix + worker->GetName();
    string topic = "COMPUTE_REPLY";
    list<TLV*> tlvList;
    tlvList.push_back(tlv);
    SendMessage(MSG_REPLY, queue, topic, tlvList);

    TEWG* tewg = (TEWG*)this->GetComputeWorker()->GetWorkflowData("TEWG");
    if (tewg)
    {
        delete tewg;
        this->GetComputeWorker()->SetWorkflowData("TEWG", NULL);
    }

    // destroy feasiblePaths ? 

    // stop out from event loop
    Action::Finish();
}



///////////////////// class Action_CreateTEWG ///////////////////////////

void* Action_CreateTEWG::GetData(string& dataName)
{
    if (dataName == "TEWG")
        return _tewg;
    return NULL;
}

void Action_CreateTEWG::Process()
{
    LOG(name<<"Process() called"<<endl);

    // run current action main logic
    string queue = MxTCE::computeThreadPrefix + worker->GetName();
    string topic = "TEWG_REQUEST";
    string expectReturnTopic = "TEWG_REPLY";
    list<TLV*> noTLVs;
    SendMessage(MSG_REQ, queue, topic, this->context, noTLVs, expectReturnTopic);
}


bool Action_CreateTEWG::ProcessChildren()
{
    LOG(name<<"ProcessChildren() called"<<endl);
    //$$$$ loop through all children to look for states

    // return true if all children have finished; otherwise false
    return Action::ProcessChildren();
}


bool Action_CreateTEWG::ProcessMessages()
{
    LOG(name<<"ProcessMessages() called"<<endl);
    // run current action logic based on received messages 
    list<Message*>::iterator itm;
    Message* msg;
    for (itm = messages.begin(); itm != messages.end(); itm++)
    {
        msg = *itm;
        if (msg->GetTopic() == "TEWG_REPLY") 
        {
            list<TLV*>& tlvList = msg->GetTLVList();
            memcpy(&_tewg, (TEWG*)tlvList.front()->value, sizeof(void*));
            // store TEWG
            this->GetComputeWorker()->SetWorkflowData("TEWG", (void*)_tewg);
        }
        //delete msg; //msg consumed 
        itm = messages.erase(itm);
    }

    //return true if all messages received and processed; otherwise false
    return Action::ProcessMessages();
}


void Action_CreateTEWG::CleanUp()
{
    LOG(name<<"CleanUp() called"<<endl);
    //$$$$ cleanup logic for current action

    // TODO: clean up TEWG data

    // cancel and cleanup children
    Action::CleanUp();    
}


void Action_CreateTEWG::Finish()
{
    LOG(name<<"Finish() called"<<endl);
    //$$$$ finish logic for current action

    // stop out from event loop
    Action::Finish();
}



///////////////////// class Action_ComputeKSP ///////////////////////////

void* Action_ComputeKSP::GetData(string& dataName)
{
    if (dataName == "FEASIBLE_PATHS")
        return _feasiblePaths;
    if (dataName == "FEASIBLE_PATHS_TEWG")
        return _feasiblePathsTewg;
    else if (dataName == "USER_CONSTRAINT")
        return _userConstraint;
    return NULL;
}

void Action_ComputeKSP::Process()
{
    LOG(name<<"Process() called"<<endl);
    TEWG* tewg = this->context.empty() ? (TEWG*)this->GetComputeWorker()->GetWorkflowData("TEWG") : 
        (TEWG*)this->GetComputeWorker()->GetContextActionData(this->context.c_str(), "Action_CreateTEWG", "TEWG");
    if (tewg == NULL)
        throw ComputeThreadException((char*)"Action_ComputeKSP::Process() No TEWG available for computation!");

    //  the user request parameters
    if (this->_userConstraint == NULL) 
    {
        list<Apimsg_user_constraint*>* userConsList = (list<Apimsg_user_constraint*>*)this->GetComputeWorker()->GetWorkflowData("USER_CONSTRAINT_LIST");
        if (userConsList == NULL || userConsList->size() == 0)
            throw ComputeThreadException((char*)"Action_ProcessRequestTopology_MP2P::Process() No USER_CONSTRAINT_LIST data from compute worker.");
        this->_userConstraint = userConsList->front();
    }
    TLink* srcLink = tewg->LookupLinkByURN(this->_userConstraint->getSrcendpoint());
    TPort* srcPort = tewg->LookupPortByURN(this->_userConstraint->getSrcendpoint());
    TNode* srcNode = tewg->LookupNodeByURN(this->_userConstraint->getSrcendpoint());
    if (srcNode == NULL)
    {
        if (srcPort != NULL)
            srcNode = (TNode*)srcPort->GetNode();
        else if (srcLink != NULL)
            srcNode= (TNode*)srcLink->GetPort()->GetNode();
        else
            throw ComputeThreadException((char*)"Action_ComputeKSP::Process() unknown source URN!");
    }
    TLink* dstLink = tewg->LookupLinkByURN(this->_userConstraint->getDestendpoint());
    TPort* dstPort = tewg->LookupPortByURN(this->_userConstraint->getDestendpoint());
    TNode* dstNode = tewg->LookupNodeByURN(this->_userConstraint->getDestendpoint());
    if (dstNode == NULL)
    {
        if (dstPort != NULL)
            dstNode= (TNode*)dstPort->GetNode();
        else if (dstLink != NULL)
            dstNode= (TNode*)dstLink->GetPort()->GetNode();
        else
            throw ComputeThreadException((char*)"Action_ComputeKSP::Process() unknown destination URN!");
    }
    u_int64_t bw = (this->_bandwidth == 0 ? (u_int64_t)this->_userConstraint->getBandwidth() : this->_bandwidth);
    //if (this->_userConstraint->getCoschedreq()&& this->_userConstraint->getCoschedreq()->getMinbandwidth() > bw)
    //    bw = this->_userConstraint->getCoschedreq()->getMinbandwidth();
    u_int32_t srcVtag, dstVtag;
    if (this->_userConstraint->getSrcvlantag() == "any" || this->_userConstraint->getSrcvlantag() == "ANY")
        srcVtag = ANY_TAG;
    else
        sscanf(this->_userConstraint->getSrcvlantag().c_str(), "%d", &srcVtag);
    if (this->_userConstraint->getDestvlantag() == "any" || this->_userConstraint->getDestvlantag() == "ANY")
        dstVtag = ANY_TAG;
    else
        sscanf(this->_userConstraint->getDestvlantag().c_str(), "%d", &dstVtag);
    TSpec tspec;
    if (this->_userConstraint->getLayer() == "3")
        tspec.Update(LINK_IFSWCAP_PSC1, LINK_IFSWCAP_ENC_PKT, bw);
    else
        tspec.Update(LINK_IFSWCAP_L2SC, LINK_IFSWCAP_ENC_ETH, bw);
    
    // routing profile - exclusion list pruning
    if (this->_userConstraint->getHopExclusionList() != NULL)
    {
        list<string>::iterator itH = this->_userConstraint->getHopExclusionList()->begin();
        for (; itH != this->_userConstraint->getHopExclusionList()->end(); itH++)
        {
            string& hopUrn = *itH;
            size_t delim = hopUrn.find("=");
            if (delim == string::npos) // excluding hop link(s)
                tewg->PruneByExclusionUrn(hopUrn);
            else // excluding vlans on the hop link(s)
            {
                string aUrn = hopUrn.substr(0, delim);
                string vlanRange = hopUrn.substr(delim+1);
                tewg->PruneHopVlans(aUrn, vlanRange);
            }
        }
    }
    
    // reservations pruning (good for simpleComputeWorker workflow)
    // for current OSCARS implementation, tcePCE should pass startTime==endTime==0
    time_t startTime = this->_userConstraint->getStarttime();
    time_t endTime = this->_userConstraint->getEndtime();
    list<TLink*>::iterator itL;
    if (startTime > 0 && endTime > startTime) {
        for (itL = tewg->GetLinks().begin(); itL != tewg->GetLinks().end(); itL++)
        {
            TLink* L = *itL;
            AggregateDeltaSeries* ads = new AggregateDeltaSeries;
            if (L->GetWorkData() == NULL)
                L->SetWorkData(new TWorkData());
            L->GetWorkData()->SetData("ADS", ads); // TODO: free ADS mem
            if (L->GetDeltaList().size() == 0)
                continue;
            list<TDelta*>::iterator itD;
            for (itD = L->GetDeltaList().begin(); itD != L->GetDeltaList().end(); itD++)
            {
                TDelta* delta = *itD;
                ads->AddDelta(delta);
            }
            TDelta* conjDelta = ads->JoinADSInWindow(startTime, endTime);
            if (conjDelta == NULL)
                continue;
            conjDelta->SetTargetResource(L);
            conjDelta->Apply();
            delete conjDelta;
        }
    }
  
    // debug:
    // tewg->LogDump();

    // prune bandwidth
    tewg->PruneByBandwidth(bw);

    TLink* ingressLink = tewg->LookupLinkByURN(this->_userConstraint->getSrcendpoint());
    if (ingressLink == NULL)
    {
        TPort* ingressPort = tewg->LookupPortByURN(this->_userConstraint->getSrcendpoint());
        if (ingressPort == NULL) 
        {
            if (srcNode->GetPorts().find("*") != srcNode->GetPorts().end())
                ingressPort = (TPort*) srcNode->GetPorts()["*"];
            else if (srcNode->GetPorts().find("**") != srcNode->GetPorts().end())
                ingressPort = (TPort*) srcNode->GetPorts()["**"];
        } 
        if (ingressPort != NULL) 
        {
            if (ingressPort->GetLinks().find("**") != ingressPort->GetLinks().end())
                ingressLink = (TLink*)ingressPort->GetLinks()["**"];
            else if (ingressPort->GetLinks().find("*") != ingressPort->GetLinks().end())
                ingressLink = (TLink*)ingressPort->GetLinks()["*"];
        }
    }
    // last resort to match an abstract node in the domain/aggregate
    if (!ingressLink)
    {
        TDomain* srcDomain = tewg->LookupDomainByURN(this->_userConstraint->getSrcendpoint());
        if (srcDomain->GetNodes().find("*") != srcDomain->GetNodes().end())
        {
            srcNode = (TNode*) srcDomain->GetNodes()["*"];
            TPort* ingressPort = NULL;
            if (srcNode->GetPorts().find("*") != srcNode->GetPorts().end())
                ingressPort = (TPort*) srcNode->GetPorts()["*"];
            else if (srcNode->GetPorts().find("**") != srcNode->GetPorts().end())
                ingressPort = (TPort*) srcNode->GetPorts()["**"];
            if (ingressPort != NULL) 
            {
                if (ingressPort->GetLinks().find("**") != ingressPort->GetLinks().end())
                    ingressLink = (TLink*)ingressPort->GetLinks()["**"];
                else if (ingressPort->GetLinks().find("*") != ingressPort->GetLinks().end())
                    ingressLink = (TLink*)ingressPort->GetLinks()["*"];
            }            
        }        
    }
    if (!ingressLink)
        throw ComputeThreadException((char*)"Action_ComputeKSP::Process() Cannot map source URN to an Ingress Edge Link!");
    if (!ingressLink->IsAvailableForTspec(tspec))
        throw ComputeThreadException((char*)"Action_ComputeKSP::Process() Ingress Edge Link is not available for requested TSpec!");
    TLink* egressLink = tewg->LookupLinkByURN(this->_userConstraint->getDestendpoint());
    if (egressLink == NULL)
    {
        TPort* egressPort = tewg->LookupPortByURN(this->_userConstraint->getDestendpoint());
        if (egressPort == NULL) 
        {
            if (dstNode->GetPorts().find("*") != dstNode->GetPorts().end())
                egressPort = (TPort*) dstNode->GetPorts()["*"];
            else if (dstNode->GetPorts().find("**") != dstNode->GetPorts().end())
                egressPort = (TPort*) dstNode->GetPorts()["**"];
        }
        if (egressPort != NULL)
        {
            if (egressPort->GetLinks().find("**") != egressPort->GetLinks().end())
                egressLink = (TLink*)egressPort->GetLinks()["**"];
            else if (egressPort->GetLinks().find("*") != egressPort->GetLinks().end())
                egressLink = (TLink*)egressPort->GetLinks()["*"];
        }
    }
    // last resort to match an abstract node in the domain/aggregate
    if (!egressLink)
    {
        TDomain* dstDomain = tewg->LookupDomainByURN(this->_userConstraint->getDestendpoint());
        if (dstDomain->GetNodes().find("*") != dstDomain->GetNodes().end())
        {
            dstNode = (TNode*) dstDomain->GetNodes()["*"];
            TPort* egressPort = NULL;
            if (dstNode->GetPorts().find("*") != dstNode->GetPorts().end())
                egressPort = (TPort*) dstNode->GetPorts()["*"];
            else if (dstNode->GetPorts().find("**") != dstNode->GetPorts().end())
                egressPort = (TPort*) dstNode->GetPorts()["**"];
            if (egressPort != NULL) 
            {
                if (egressPort->GetLinks().find("**") != egressPort->GetLinks().end())
                    egressLink = (TLink*)egressPort->GetLinks()["**"];
                else if (egressPort->GetLinks().find("*") != egressPort->GetLinks().end())
                    egressLink = (TLink*)egressPort->GetLinks()["*"];
            }            
        }        
    }
    if (!egressLink)
        throw ComputeThreadException((char*)"Action_ComputeKSP::Process() Cannot map destination URN to an Egress Edge Link!");
    if (!egressLink->IsAvailableForTspec(tspec))
        throw ComputeThreadException((char*)"Action_ComputeKSP::Process() Egress Edge Link is not available for requested TSpec!");

    // compute KSP
    vector<TPath*> KSP;
    try {
        tewg->ComputeKShortestPaths(srcNode, dstNode, tewg->GetNodes().size()*2>MAX_KSP_K?MAX_KSP_K:tewg->GetNodes().size()*2, KSP);
    } catch (TCEException e) {
        LOG_DEBUG("Action_ComputeKSP::Process raised exception: " << e.GetMessage() <<endl);
        throw ComputeThreadException(e.GetMessage());
    }

    if (KSP.size() == 0)
    {
        LOG_DEBUG("Action_ComputeKSP::Process() No KSP found after bandwidh pruning!" <<endl);
        throw ComputeThreadException((char*)"Action_ComputeKSP::Process() No KSP found after bandwidh pruning!");
    }

    // verify constraints with switchingType / layer adaptation / VLAN etc.
    vector<TPath*>::iterator itP = KSP.begin(); 
    while (itP != KSP.end())
    {
        if (!(*ingressLink == *(*itP)->GetPath().front()))
        {
            // create artificial source node and link to handle edge ingress link
            if (ingressLink->GetRemoteEnd() == NULL)
            {
                TLink* newSrcLink = ingressLink->Clone();
                TNode* newSrcNode = ((TNode*)ingressLink->GetPort()->GetNode())->Clone();
                newSrcNode->GetLocalLinks().push_back(newSrcLink);
                newSrcNode->GetRemoteLinks().push_back(ingressLink);
                ((TNode*)ingressLink->GetPort()->GetNode())->GetRemoteLinks().push_back(newSrcLink);
                ingressLink->SetRemoteEnd(newSrcNode);
                ingressLink->SetRemoteLink(newSrcLink);
                newSrcLink->SetRemoteLink(ingressLink);
                newSrcLink->SetLocalEnd(newSrcNode);
                newSrcLink->SetRemoteEnd((TNode*)ingressLink->GetPort()->GetNode());
                tewg->GetLinks().push_back(newSrcLink);
                tewg->GetNodes().push_back(newSrcNode);
                newSrcLink->SetWorkData(new TWorkData());
                newSrcNode->SetWorkData(new TWorkData());
            }
            (*itP)->GetPath().push_front(ingressLink);
        }
        if (!(*egressLink == *(*itP)->GetPath().back()))
        {
            // create artificial destination node and link to handle edge ingress link
            if (egressLink->GetRemoteEnd() == NULL)
            {
                TLink* newDstLink = egressLink->Clone();
                TNode* newDstNode = ((TNode*)egressLink->GetPort()->GetNode())->Clone();
                newDstNode->GetLocalLinks().push_back(newDstLink);
                newDstNode->GetRemoteLinks().push_back(egressLink);
                ((TNode*)egressLink->GetPort()->GetNode())->GetRemoteLinks().push_back(newDstLink);
                egressLink->SetRemoteEnd(newDstNode);
                egressLink->SetRemoteLink(newDstLink);
                newDstLink->SetRemoteLink(egressLink);
                newDstLink->SetLocalEnd(newDstNode);
                newDstLink->SetRemoteEnd((TNode*)egressLink->GetPort()->GetNode());
                tewg->GetLinks().push_back(newDstLink);
                tewg->GetNodes().push_back(newDstNode);
                newDstLink->SetWorkData(new TWorkData());
                newDstNode->SetWorkData(new TWorkData());
            }
            (*itP)->GetPath().push_back(egressLink);
        }

        // TODO: ? special handling for old OSCARS L2SC --> PSC edge adaptation: add artificial IACD for  (ingress <-> 1st-hop and egress <-> last-hop)

        // verify TE constraints
        TServiceSpec ingTSS, egrTSS;
        ingTSS.Update(tspec.SWtype, tspec.ENCtype, tspec.Bandwidth);
        ingTSS.GetVlanSet().AddTag(srcVtag);
        egrTSS.Update(tspec.SWtype, tspec.ENCtype, tspec.Bandwidth);
        egrTSS.GetVlanSet().AddTag(dstVtag);
        (*itP)->ExpandWithRemoteLinks();
        // verify loop free
        if (!(*itP)->VerifyLoopFree())
        {
            TPath* path2erase = *itP;
            itP = KSP.erase(itP);
            delete path2erase;
            continue;                
        }
        // verify hop inclusion list
        if (this->_userConstraint->getHopInclusionList() != NULL)
        {            
            if (!(*itP)->VerifyHopInclusionList(*this->_userConstraint->getHopInclusionList()))
            {
                TPath* path2erase = *itP;
                itP = KSP.erase(itP);
                delete path2erase;
                continue;
            }
        }
        // verifying TE constraints
        if (!(*itP)->VerifyTEConstraints(ingTSS, egrTSS))
        {
            TPath* path2erase = *itP;
            itP = KSP.erase(itP);
            delete path2erase;
        }
        else
        {
            if (this->_feasiblePathsTewg == NULL)
            {
                this->_feasiblePathsTewg = new vector<TPath*>;
                this->GetComputeWorker()->SetWorkflowData("FEASIBLE_PATHS_TEWG", this->_feasiblePathsTewg);
            }            
            this->_feasiblePathsTewg->push_back(*itP);

            if (this->_feasiblePaths == NULL)
            {
                this->_feasiblePaths = new vector<TPath*>;
                this->GetComputeWorker()->SetWorkflowData("FEASIBLE_PATHS", this->_feasiblePaths);
            }
			// make a copy of TPath from work set. Then do twists on the copy to satisfy reply format.
			// Caution: Clone() will inherit the orignal localEnd and remoteEnd nodes from work set.
            TPath* feasiblePath = (*itP)->Clone();
            // check whether BAG is requested
            if (yesComputeBAG() && _userConstraint->getCoschedreq() && _userConstraint->getCoschedreq()->getBandwidthavaigraph()) 
            {
                BandwidthAvailabilityGraph* bag = (*itP)->CreatePathBAG(this->_userConstraint->getCoschedreq()->getStarttime(), 
                    this->_userConstraint->getCoschedreq()->getEndtime());
                if (bag != NULL) 
                {
                    feasiblePath->SetBAG(bag);
                    (*itP)->SetBAG(NULL);
                }
                if (_userConstraint->getCoschedreq()->getRequireLinkBag())
                {
                    (*itP)->CreateLinkBAG(_userConstraint->getCoschedreq()->getStarttime(), _userConstraint->getCoschedreq()->getEndtime());
                    // copying over link BAGs
                    list<TLink*>::iterator itLF = feasiblePath->GetPath().begin();
                    for (itL = (*itP)->GetPath().begin(); itL != (*itP)->GetPath().end(); itL++) 
                    {
                        (*itLF)->SetBAG((*itL)->GetBAG());
                        (*itL)->SetBAG(NULL);
                        itLF++;
                    }
                }
            }
            // modify bandwidth to service bw
            for (itL = feasiblePath->GetPath().begin(); itL != feasiblePath->GetPath().end(); itL++)
                (*itL)->SetMaxBandwidth(bw);
            // modify layer spec info
            feasiblePath->UpdateLayerSpecInfo(ingTSS, egrTSS, this->_userConstraint->getPreserveVlanAvailabilityRange());
            this->_feasiblePaths->push_back(feasiblePath);
            itP++;
        }
    }
    // store a list of ordered result paths 
    if (this->_feasiblePaths == NULL || this->_feasiblePaths->size() == 0)
    {
        throw ComputeThreadException((char*)"Action_ComputeKSP::Process() No feasible path found after KSP being applied with TE constraints!");
    }
    sort(_feasiblePaths->begin(), _feasiblePaths->end(), cmp_tpath);
    for (itP = _feasiblePaths->begin(); itP != _feasiblePaths->end(); itP++)
    {
        // debugging output
        //(*itP)->LogDump();
    }
}


bool Action_ComputeKSP::ProcessChildren()
{
    LOG(name<<"ProcessChildren() called"<<endl);
    //$$$$ loop through all children to look for states

    // return true if all children have finished; otherwise false
    return Action::ProcessChildren();
}


bool Action_ComputeKSP::ProcessMessages()
{
    LOG(name<<"ProcessMessages() called"<<endl);
    //$$$$ process messages if received
    //$$$$ run current action logic based on received messages 

    //return true if all messages received and processed; otherwise false
    return Action::ProcessMessages();
}


void Action_ComputeKSP::CleanUp()
{
    LOG(name<<"CleanUp() called"<<endl);
    //$$$$ cleanup logic for current action

    // TODO: if (status == _Failed) set failure status to worker thread ? Or leave to Action_ProcessRequestTopology::Finish

    // cancel and cleanup children
    Action::CleanUp();
}


void Action_ComputeKSP::Finish()
{
    LOG(name<<"Finish() called"<<endl);
    //$$$$ finish logic for current action

    // stop out from event loop
    Action::Finish();
}


///////////////////// class Action_FinalizeServiceTopology ///////////////////////////

void Action_FinalizeServiceTopology::Process()
{
    LOG(name<<"Process() called"<<endl);

    vector<TPath*>* feasiblePaths = (vector<TPath*>*)this->GetComputeWorker()->GetWorkflowData("FEASIBLE_PATHS");

    if (feasiblePaths != NULL)
    {
        if (feasiblePaths->size() == 0)
            throw ComputeThreadException((char*)"Action_FinalizeServiceTopology::Process() No feasible path found!");
        LOG_DEBUG("Feasible Paths and Schedules: "<<endl);
        vector<TPath*>::iterator itP = feasiblePaths->begin();
        for (; itP != feasiblePaths->end(); itP++)
        {
            (*itP)->LogDump();
        }
    }
    else 
    {
        throw ComputeThreadException((char*)"Action_FinalizeServiceTopology::Process() No path found!");
    }
}


bool Action_FinalizeServiceTopology::ProcessChildren()
{
    LOG(name<<"ProcessChildren() called"<<endl);
    //$$$$ loop through all children to look for states

    // return true if all children have finished; otherwise false
    return Action::ProcessChildren();
}


bool Action_FinalizeServiceTopology::ProcessMessages()
{
    LOG(name<<"ProcessMessages() called"<<endl);
    //$$$$ process messages if received
    //$$$$ run current action logic based on received messages 

    //return true if all messages received and processed; otherwise false
    return Action::ProcessMessages();
}


void Action_FinalizeServiceTopology::CleanUp()
{
    LOG(name<<"CleanUp() called"<<endl);
    //$$$$ cleanup logic for current action

    // TODO: clean up work data such as KSP, feasiblePaths and ATS 

    // cancel and cleanup children
    Action::CleanUp();
}


void Action_FinalizeServiceTopology::Finish()
{
    LOG(name<<"Finish() called"<<endl);
    //$$$$ finish logic for current action

    // stop out from event loop
    Action::Finish();
}

///////////////////////////////////////////////////////////////////////////

///////////////////// class Action_CreateOrderedATS ///////////////////////////

///////////////////////////////////////////////////////////////////////////

inline void Action_CreateOrderedATS::AddUniqueTimePoint(list<time_t>* ats, time_t t)
{
    list<time_t>::iterator it;
    for (it = ats->begin(); it != ats->end(); it++)
    {
        if ((*it) == t)
            return;
    }
    ats->push_back(t);
}

void* Action_CreateOrderedATS::GetData(string& dataName)
{
    if (dataName == "ORDERED_ATS")
        return _orderedATS;
    return NULL;
}

void Action_CreateOrderedATS::Process()
{
    LOG(name<<"Process() called"<<endl);

    assert(_bandwidth > 0 && _volume > 0);

    // get the K-Paths from previous computation: use the ones pointing to orinal links in TEWG instead of the cloned feasiblePaths
    vector<TPath*>* KSP = this->context.empty() ? (vector<TPath*>*)this->GetComputeWorker()->GetWorkflowData("FEASIBLE_PATHS_TEWG") 
        : (vector<TPath*>*)this->GetComputeWorker()->GetContextActionData(this->context.c_str(), "Action_ComputeKSP", "FEASIBLE_PATHS_TEWG");

    if (KSP == NULL || KSP->size() == 0)
        throw ComputeThreadException((char*)"Action_CreateOrderedATS::Process() Empty KSP list: no path found!");

    TEWG* tewg = this->context.empty() ? (TEWG*)this->GetComputeWorker()->GetWorkflowData("TEWG") : 
        (TEWG*)this->GetComputeWorker()->GetContextActionData(this->context.c_str(), "Action_CreateTEWG", "TEWG");

    vector<TPath*>::iterator itP;
    list<TLink*>::iterator itL;
    for (itL = tewg->GetLinks().begin(); itL != tewg->GetLinks().end(); itL++)
    {
        if ((*itL)->GetWorkData() == NULL)
            (*itL)->SetWorkData(new TWorkData);
        int* piVal = new int(0);
        (*itL)->GetWorkData()->SetData("ATS_Order_Counter", (void*)piVal);
    }
    for (itP = KSP->begin(); itP != KSP->end(); itP++)
    {
        TPath* P = *itP;
        for (itL = P->GetPath().begin(); itL != P->GetPath().end(); itL++)
        {
            int* piVal = (int*)((*itL)->GetWorkData()->GetData("ATS_Order_Counter"));
            (*piVal)++;
        }
    }

    list<TLink*> orderedLinks;
    list<TLink*>::iterator itL2;
    for (itL = tewg->GetLinks().begin(); itL != tewg->GetLinks().end(); itL++)
    {
        if ((*itL)->GetWorkData()->GetInt("ATS_Order_Counter") == 0)
            continue;
        int iVal = (*itL)->GetWorkData()->GetInt("ATS_Order_Counter");        
        for (itL2 = orderedLinks.begin(); itL2 != orderedLinks.end(); itL2++)
        {
            if (iVal > (*itL2)->GetWorkData()->GetInt("ATS_Order_Counter"))
                orderedLinks.insert(itL2, *itL);
        }
        if (itL2 == orderedLinks.end())
            orderedLinks.push_back(*itL);
    }

    // clean up
    for (itL = tewg->GetLinks().begin(); itL != tewg->GetLinks().end(); itL++)
    {
        if ((*itL)->GetWorkData()->GetData("ATS_Order_Counter") != NULL)
            delete (int*)(*itL)->GetWorkData()->GetData("ATS_Order_Counter");
        (*itL)->GetWorkData()->SetData("ATS_Order_Counter", NULL);
    }

    _orderedATS = new list<time_t>;
    for (itL2 = orderedLinks.begin(); itL2 != orderedLinks.end(); itL2++)
    {
        TLink* L = *itL2;
        //$$ create Link ADS
        AggregateDeltaSeries* ads = new AggregateDeltaSeries;
        (*itL2)->GetWorkData()->SetData("ADS", ads);
        if (L->GetDeltaList().size() == 0)
            continue;
        list<TDelta*>::iterator itD;
        for (itD = L->GetDeltaList().begin(); itD != L->GetDeltaList().end(); itD++)
        {
            TDelta* delta = *itD;
            ads->AddDelta(delta);
        }
        //$$ add qualified link time points to orderedATS
        time_t t_start = time(0); 
        time_t t_end = t_start;
        time_t t_next = 0; // for sliding t_start
        u_int64_t maxRemainBW = L->GetMaxReservableBandwidth() - ((TLinkDelta*)ads->GetADS().front())->GetBandwidth();

        for (itD = ads->GetADS().begin(); itD != ads->GetADS().end(); itD++)
        {
            TDelta* delta = *itD;
            t_end = delta->GetStartTime();
            if (t_next == 0 || t_next <= t_start)
                t_next = t_end;

            //$$ judge whether link resource between (t_start, t_end) can satisfy request (before current delta)
            if (maxRemainBW*(t_end - t_start) > this->GetReqVolume())
            {
                AddUniqueTimePoint(_orderedATS, t_start);
                t_start = t_next;
            }

            if (maxRemainBW > L->GetMaxReservableBandwidth() - (((TLinkDelta*)delta)->GetBandwidth()))
                maxRemainBW = L->GetMaxReservableBandwidth() - (((TLinkDelta*)delta)->GetBandwidth());
            t_end = delta->GetEndTime();
            if (t_next <= t_start)
                t_next = t_end;

            //$$ judge whether link resource between (t_start, t_end) can satisfy request (including current delta)
            //     with both ReqVolume and ReqBandwidth crieria
            if (maxRemainBW < this->GetReqBandwidth())
            {
                t_start = t_end = delta->GetEndTime();
                maxRemainBW = L->GetMaxReservableBandwidth();
                continue;
            }
            if (maxRemainBW*(t_end - t_start) > this->GetReqVolume())
            {
                AddUniqueTimePoint(_orderedATS, t_start);
                t_start = t_next;
            }
        }
        if (_orderedATS->size() >= MAX_ATS_SIZE)
            break;
    }


    time_t t0 = time(0);
    list<time_t>::iterator itT = _orderedATS->begin();
    for (; itT != _orderedATS->end(); itT++)
    {
        // trim ATS before current time points
        if (*itT <= time(0)) {
            itT = _orderedATS->erase(itT);
            continue;
        }
        // trim ATS out of schedule time points
        if (_userConstraint != NULL && _userConstraint->getFlexSchedules() != NULL) 
        {
            list<TSchedule*>::iterator itS = _userConstraint->getFlexSchedules()->begin();
            for (; itS != _userConstraint->getFlexSchedules()->end(); itS++)
            {
                TSchedule * schedule = *itS;
                if (schedule->WithinSchedule(*itT))
                {
                    itT = _orderedATS->erase(itT);
                    break;
                }                    
            }
        }
    }
    // add current time 
    _orderedATS->push_front(t0);

    // remove unqualified schedule starting times from the ATS
    list<TSchedule*>::iterator itS;
    if (_userConstraint != NULL && _userConstraint->getFlexSchedules() != NULL 
        && _userConstraint->getFlexSchedules()->size() > 0 && _volume != 0 && _bandwidth != 0) 
    {
        for (itT = _orderedATS->begin(); itT != _orderedATS->end(); itT++)
        {
            for (itS = _userConstraint->getFlexSchedules()->begin(); itS != _userConstraint->getFlexSchedules()->end(); itS++)
            {
                TSchedule * schedule = *itS;
                if (schedule->WithinSchedule(*itT) && (*itT)-schedule->GetEndTime() >= (_volume/_bandwidth))
                {
                    break;
                }                    
            }
            if (itS == _userConstraint->getFlexSchedules()->end()) 
            {
                itT = _orderedATS->erase(itT);
            }
        }
        // add flex schedule starting points to ATS
        for (itS = _userConstraint->getFlexSchedules()->begin(); itS != _userConstraint->getFlexSchedules()->end(); itS++)
        {
            if ((*itS)->GetStartTime() >= t0 && (*itS)->GetEndTime()-(*itS)->GetStartTime() >= (_volume/_bandwidth))
            {
                AddUniqueTimePoint(_orderedATS, (*itS)->GetStartTime());
            }
        }
    }
}


bool Action_CreateOrderedATS::ProcessChildren()
{
    LOG(name<<"ProcessChildren() called"<<endl);
    //$$$$ loop through all children to look for states

    // return true if all children have finished; otherwise false
    return Action::ProcessChildren();
}


bool Action_CreateOrderedATS::ProcessMessages()
{
    LOG(name<<"ProcessMessages() called"<<endl);
    //$$$$ process messages if received
    //$$$$ run current action logic based on received messages 

    //return true if all messages received and processed; otherwise false
    return Action::ProcessMessages();
}


void Action_CreateOrderedATS::CleanUp()
{
    LOG(name<<"CleanUp() called"<<endl);
    //$$$$ cleanup logic for current action
    // TODO: clean up link AggregateDeltaSeries in TEWG
    // TODO: clean up local _orderedATS

    // cancel and cleanup children
    Action::CleanUp();
}


void Action_CreateOrderedATS::Finish()
{
    LOG(name<<"Finish() called"<<endl);
    //$$$$ finish logic for current action

    // stop out from event loop
    Action::Finish();
}


///////////////////// class Action_ComputeSchedulesWithKSP ///////////////////////////

void* Action_ComputeSchedulesWithKSP::GetData(string& dataName)
{
    if (dataName == "FEASIBLE_PATHS")
        return _feasiblePaths;
    return NULL;
}


void Action_ComputeSchedulesWithKSP::Process()
{
    LOG(name<<"Process() called"<<endl);
    assert(_bandwidth > 0 && _volume > 0);

    TEWG* tewg = this->context.empty() ? (TEWG*)this->GetComputeWorker()->GetWorkflowData("TEWG") :
        (TEWG*)this->GetComputeWorker()->GetContextActionData(this->context.c_str(), "Action_CreateTEWG", "TEWG");

    if (tewg == NULL)
        throw ComputeThreadException((char*)"Action_ComputeSchedulesWithKSP::Process() No TEWG available for computation!");

    if (_userConstraint == NULL)
    {
        list<Apimsg_user_constraint*>* userConsList = (list<Apimsg_user_constraint*>*)this->GetComputeWorker()->GetWorkflowData("USER_CONSTRAINT_LIST");
        if (userConsList == NULL || userConsList->size() == 0)
            throw ComputeThreadException((char*)"Action_ProcessRequestTopology_MP2P::Process() No USER_CONSTRAINT_LIST data from compute worker.");
        this->_userConstraint = userConsList->front();
    }

    string actionName = "Action_CreateOrderedATS";
    list<time_t>* orderedATS = (list<time_t>*)this->GetComputeWorker()->GetContextActionData(this->context, actionName, "ORDERED_ATS");
    if (tewg == NULL)
        throw ComputeThreadException((char*)"Action_ComputeSchedulesWithKSP::Process() No Ordered Aggregate Time Series available for computation!");

    TLink* srcLink = tewg->LookupLinkByURN(this->_userConstraint->getSrcendpoint());
    TPort* srcPort = tewg->LookupPortByURN(this->_userConstraint->getSrcendpoint());
    TNode* srcNode = tewg->LookupNodeByURN(this->_userConstraint->getSrcendpoint());
    if (srcNode == NULL)
    {
        if (srcPort != NULL)
            srcNode = (TNode*)srcPort->GetNode();
        else if (srcLink != NULL)
            srcNode= (TNode*)srcLink->GetPort()->GetNode();
        else
            throw ComputeThreadException((char*)"Action_ComputeSchedulesWithKSP::Process() unknown source URN!");
    }
    TLink* dstLink = tewg->LookupLinkByURN(this->_userConstraint->getDestendpoint());
    TPort* dstPort = tewg->LookupPortByURN(this->_userConstraint->getDestendpoint());
    TNode* dstNode = tewg->LookupNodeByURN(this->_userConstraint->getDestendpoint());
    if (dstNode == NULL)
    {
        if (dstPort != NULL)
            dstNode= (TNode*)dstPort->GetNode();
        else if (dstLink != NULL)
            dstNode= (TNode*)dstLink->GetPort()->GetNode();
        else
            throw ComputeThreadException((char*)"Action_ComputeSchedulesWithKSP::Process() unknown destination URN!");
    }
        
    u_int64_t bw = (this->GetReqBandwidth() == 0 ? _userConstraint->getBandwidth():this->GetReqBandwidth());
    u_int32_t duration = (this->GetReqVolume() == 0 ? _userConstraint->getEndtime()-_userConstraint->getStarttime():this->GetReqVolume()/this->GetReqBandwidth());
    if (_userConstraint->getCoschedreq()&& _userConstraint->getCoschedreq()->getMinbandwidth() > bw)
        bw = _userConstraint->getCoschedreq()->getMinbandwidth();
    u_int32_t srcVtag, dstVtag;
    if (_userConstraint->getSrcvlantag() == "any" || _userConstraint->getSrcvlantag() == "ANY")
        srcVtag = ANY_TAG;
    else
        sscanf(_userConstraint->getSrcvlantag().c_str(), "%d", &srcVtag);
    if (_userConstraint->getDestvlantag() == "any" || _userConstraint->getDestvlantag() == "ANY")
        dstVtag = ANY_TAG;
    else
        sscanf(_userConstraint->getDestvlantag().c_str(), "%d", &dstVtag);
    TSpec tspec;
    if (_userConstraint->getLayer() == "3")
        tspec.Update(LINK_IFSWCAP_PSC1, LINK_IFSWCAP_ENC_PKT, bw);
    else
        tspec.Update(LINK_IFSWCAP_L2SC, LINK_IFSWCAP_ENC_ETH, bw);

    if (_feasiblePaths == NULL)
    {
        _feasiblePaths = new vector<TPath*>;
    }

    vector<TPath*> KSP;
    list<time_t>::iterator itT = orderedATS->begin();
    for (; itT != orderedATS->end(); itT++)
    {
        // routing profile - exclusion list pruning
        if (this->_userConstraint->getHopExclusionList() != NULL)
        {
            list<string>::iterator itH = this->_userConstraint->getHopExclusionList()->begin();
            for (; itH != this->_userConstraint->getHopExclusionList()->end(); itH++)
            {
                string& hopUrn = *itH;
                size_t delim = hopUrn.find("=");
                if (delim == string::npos) // excluding hop link(s)
                    tewg->PruneByExclusionUrn(hopUrn);
                else // excluding vlans on the hop link(s)
                {
                    string aUrn = hopUrn.substr(0, delim);
                    string vlanRange = hopUrn.substr(delim+1);
                    tewg->PruneHopVlans(aUrn, vlanRange);
                }
            }        
        }
        // applying reservation data
        time_t startTime = (*itT);
        time_t endTime = startTime + duration;
        list<TLink*>::iterator itL;
        for (itL = tewg->GetLinks().begin(); itL != tewg->GetLinks().end(); itL++)
        {
            // $$ get conjoined ADS delta list in window
            AggregateDeltaSeries* ads = (AggregateDeltaSeries*)((*itL)->GetWorkData()->GetData("ADS"));
            if (ads == NULL)
                continue;
            TDelta* conjDelta = ads->JoinADSInWindow(startTime, endTime);
            if (conjDelta == NULL)
                continue;
            (*itL)->GetWorkData()->SetData("CONJOINED_DELTA", conjDelta);
            // $$ deduct resource by conjoined delta
            conjDelta->SetTargetResource(*itL);
            conjDelta->Apply();
        }
        // pruning bandwidth
        tewg->PruneByBandwidth(this->GetReqBandwidth());
        
        TLink* ingressLink = tewg->LookupLinkByURN(_userConstraint->getSrcendpoint());
        if (ingressLink == NULL)
        {
            TPort* ingressPort = tewg->LookupPortByURN(this->_userConstraint->getSrcendpoint());
            if (ingressPort == NULL)
            {
                if (srcNode->GetPorts().find("*") != srcNode->GetPorts().end())
                    ingressPort = (TPort*)srcNode->GetPorts()["*"];
                else if (srcNode->GetPorts().find("**") != srcNode->GetPorts().end())
                    ingressPort = (TPort*)srcNode->GetPorts()["**"];
            }
            if (ingressPort != NULL)
            {
                if (ingressPort->GetLinks().find("**") != ingressPort->GetLinks().end())
                    ingressLink = (TLink*)ingressPort->GetLinks()["**"];
                else if (ingressPort->GetLinks().find("*") != ingressPort->GetLinks().end())
                    ingressLink = (TLink*)ingressPort->GetLinks()["*"];
            }
        }
        // last resort to match an abstract node in the domain/aggregate
        if (!ingressLink)
        {
            TDomain* srcDomain = tewg->LookupDomainByURN(this->_userConstraint->getSrcendpoint());
            if (srcDomain->GetNodes().find("*") != srcDomain->GetNodes().end())
            {
                srcNode = (TNode*) srcDomain->GetNodes()["*"];
                TPort* ingressPort = NULL;
                if (srcNode->GetPorts().find("*") != srcNode->GetPorts().end())
                    ingressPort = (TPort*) srcNode->GetPorts()["*"];
                else if (srcNode->GetPorts().find("**") != srcNode->GetPorts().end())
                    ingressPort = (TPort*) srcNode->GetPorts()["**"];
                if (ingressPort != NULL) 
                {
                    if (ingressPort->GetLinks().find("**") != ingressPort->GetLinks().end())
                        ingressLink = (TLink*)ingressPort->GetLinks()["**"];
                    else if (ingressPort->GetLinks().find("*") != ingressPort->GetLinks().end())
                        ingressLink = (TLink*)ingressPort->GetLinks()["*"];
                }            
            }        
        }
        if (!ingressLink)
            throw ComputeThreadException((char*)"Action_ComputeSchedulesWithKSP::Process() Cannot map source URN to an Ingress Edge Link!");
        if (!ingressLink->IsAvailableForTspec(tspec))
            throw ComputeThreadException((char*)"Action_ComputeSchedulesWithKSP::Process() Ingress Edge Link is not available for requested TSpec!");
        TLink* egressLink = tewg->LookupLinkByURN(_userConstraint->getDestendpoint());
        if (egressLink == NULL)
        {
            TPort* egressPort = tewg->LookupPortByURN(this->_userConstraint->getDestendpoint());
            if (egressPort == NULL)
            {
                if (dstNode->GetPorts().find("*") != dstNode->GetPorts().end())
                    egressPort = (TPort*)dstNode->GetPorts()["*"];
                else if (dstNode->GetPorts().find("**") != dstNode->GetPorts().end())
                    egressPort = (TPort*)dstNode->GetPorts()["**"];
            }
            if (egressPort != NULL)
            {
                if (egressPort->GetLinks().find("**") != egressPort->GetLinks().end())
                    egressLink = (TLink*)egressPort->GetLinks()["**"];
                else if (egressPort->GetLinks().find("*") != egressPort->GetLinks().end())
                    egressLink = (TLink*)egressPort->GetLinks()["*"];
            }
        }
        // last resort to match an abstract node in the domain/aggregate
        if (!egressLink)
        {
            TDomain* dstDomain = tewg->LookupDomainByURN(this->_userConstraint->getDestendpoint());
            if (dstDomain->GetNodes().find("*") != dstDomain->GetNodes().end())
            {
                dstNode = (TNode*) dstDomain->GetNodes()["*"];
                TPort* egressPort = NULL;
                if (dstNode->GetPorts().find("*") != dstNode->GetPorts().end())
                    egressPort = (TPort*) dstNode->GetPorts()["*"];
                else if (dstNode->GetPorts().find("**") != dstNode->GetPorts().end())
                    egressPort = (TPort*) dstNode->GetPorts()["**"];
                if (egressPort != NULL) 
                {
                    if (egressPort->GetLinks().find("**") != egressPort->GetLinks().end())
                        egressLink = (TLink*)egressPort->GetLinks()["**"];
                    else if (egressPort->GetLinks().find("*") != egressPort->GetLinks().end())
                        egressLink = (TLink*)egressPort->GetLinks()["*"];
                }            
            }        
        }
        if (!egressLink)
            throw ComputeThreadException((char*)"Action_ComputeSchedulesWithKSP::Process() Cannot map destination URN to an Egress Edge Link!");
        if (!egressLink->IsAvailableForTspec(tspec))
            throw ComputeThreadException((char*)"Action_ComputeSchedulesWithKSP::Process() Egress Edge Link is not available for requested TSpec!");
        
        // compute KSP
        KSP.clear();
        try {
            tewg->ComputeKShortestPaths(srcNode, dstNode, tewg->GetNodes().size()*2>MAX_KSP_K?MAX_KSP_K:tewg->GetNodes().size()*2, KSP);
        } catch (TCEException e) {
            //some debug logging here but do not throw exception upward
            LOG_DEBUG("Action_ComputeSchedulesWithKSP::Process() at TimePoint=" << startTime << " raised exception: " << e.GetMessage() <<endl);
        }
        // verify constraints with switchingType / layer adaptation / VLAN etc.
        vector<TPath*>::iterator itP = KSP.begin(); 
        while (itP != KSP.end())
        {
            if (!(*ingressLink == *(*itP)->GetPath().front()))
            {
                // create artificial source node and link to handle edge ingress link
                if (ingressLink->GetRemoteEnd() == NULL)
                {
                    TLink* newSrcLink = ingressLink->Clone();
                    TNode* newSrcNode = ((TNode*)ingressLink->GetPort()->GetNode())->Clone();
                    newSrcNode->GetLocalLinks().push_back(newSrcLink);
                    newSrcNode->GetRemoteLinks().push_back(ingressLink);
                    ((TNode*)ingressLink->GetPort()->GetNode())->GetRemoteLinks().push_back(newSrcLink);
                    ingressLink->SetRemoteEnd(newSrcNode);
                    ingressLink->SetRemoteLink(newSrcLink);
                    newSrcLink->SetRemoteLink(ingressLink);
                    newSrcLink->SetLocalEnd(newSrcNode);
                    newSrcLink->SetRemoteEnd((TNode*)ingressLink->GetPort()->GetNode());
                    tewg->GetLinks().push_back(newSrcLink);
                    tewg->GetNodes().push_back(newSrcNode);
                    newSrcLink->SetWorkData(new TWorkData());
                    newSrcNode->SetWorkData(new TWorkData());
                }
                (*itP)->GetPath().push_front(ingressLink);
            }
            if (!(*egressLink == *(*itP)->GetPath().back()))
            {
                // create artificial destination node and link to handle edge ingress link
                if (egressLink->GetRemoteEnd() == NULL)
                {
                    TLink* newDstLink = egressLink->Clone();
                    TNode* newDstNode = ((TNode*)egressLink->GetPort()->GetNode())->Clone();
                    newDstNode->GetLocalLinks().push_back(newDstLink);
                    newDstNode->GetRemoteLinks().push_back(egressLink);
                    ((TNode*)egressLink->GetPort()->GetNode())->GetRemoteLinks().push_back(newDstLink);
                    egressLink->SetRemoteEnd(newDstNode);
                    egressLink->SetRemoteLink(newDstLink);
                    newDstLink->SetRemoteLink(egressLink);
                    newDstLink->SetLocalEnd(newDstNode);
                    newDstLink->SetRemoteEnd((TNode*)egressLink->GetPort()->GetNode());
                    tewg->GetLinks().push_back(newDstLink);
                    tewg->GetNodes().push_back(newDstNode);
                    newDstLink->SetWorkData(new TWorkData());
                    newDstNode->SetWorkData(new TWorkData());
                }
                (*itP)->GetPath().push_back(egressLink);
            }
                
            TServiceSpec ingTSS, egrTSS;
            ingTSS.Update(tspec.SWtype, tspec.ENCtype, tspec.Bandwidth);
            ingTSS.GetVlanSet().AddTag(srcVtag);
            egrTSS.Update(tspec.SWtype, tspec.ENCtype, tspec.Bandwidth);
            egrTSS.GetVlanSet().AddTag(dstVtag);
            (*itP)->ExpandWithRemoteLinks();
            // verify loop free
            if (!(*itP)->VerifyLoopFree())
            {
                TPath* path2erase = *itP;
                itP = KSP.erase(itP);
                delete path2erase;
                continue;                
            }
            // verify hop inclusion list
            if (this->_userConstraint->getHopInclusionList() != NULL)
            {            
                if (!(*itP)->VerifyHopInclusionList(*this->_userConstraint->getHopInclusionList()))
                {
                    TPath* path2erase = *itP;
                    itP = KSP.erase(itP);
                    delete path2erase;
                    continue;
                }
            }
            // verify TE constraints
            if (!(*itP)->VerifyTEConstraints(ingTSS, egrTSS))
            {
                TPath* path2erase = *itP;
                itP = KSP.erase(itP);
                delete path2erase;
            }
            else
            {
                
                // TODO:  the time window may be increased if available 
                TSchedule* schedule = new TSchedule(startTime, endTime);
                vector<TPath*>::iterator itFP = _feasiblePaths->begin();
                for (; itFP != _feasiblePaths->end(); itFP++)
                {
                    if ((*(*itFP)) == (*(*itP)))
                    {
                        (*itFP)->GetSchedules().push_back(schedule);
                        break;
                    }
                }
                if (itFP != _feasiblePaths->end())
                {
                    itP++;
                    continue; 
                }
                // make a copy of TPath from work set. Then do twists on the copy to satisfy reply format.
                // Caution: Clone() will inherit the orignal localEnd and remoteEnd nodes from work set.
                TPath* feasiblePath = (*itP)->Clone();
                // check whether BAG is requested
                if (yesComputeBAG() && _userConstraint->getCoschedreq() && _userConstraint->getCoschedreq()->getBandwidthavaigraph()) 
                {
                    BandwidthAvailabilityGraph* bag = (*itP)->CreatePathBAG(_userConstraint->getCoschedreq()->getStarttime(), 
                        _userConstraint->getCoschedreq()->getEndtime());
                    if (bag != NULL) 
                    {
                        feasiblePath->SetBAG(bag);
                        (*itP)->SetBAG(NULL);
                    }
                    if (_userConstraint->getCoschedreq()->getRequireLinkBag())
                    {
                        (*itP)->CreateLinkBAG(_userConstraint->getCoschedreq()->getStarttime(), _userConstraint->getCoschedreq()->getEndtime());
                        // copying over link BAGs
                        list<TLink*>::iterator itLF = feasiblePath->GetPath().begin();
                        for (itL = (*itP)->GetPath().begin(); itL != (*itP)->GetPath().end(); itL++) 
                        {
                            (*itLF)->SetBAG((*itL)->GetBAG());
                            (*itL)->SetBAG(NULL);
                            itLF++;
                        }
                    }
                }
                feasiblePath->GetSchedules().push_back(schedule);
                // modify bandwidth to service bw
                for (itL = feasiblePath->GetPath().begin(); itL != feasiblePath->GetPath().end(); itL++)
                {
                    (*itL)->SetMaxBandwidth(bw);
                    (*itL)->SetMaxReservableBandwidth(bw);
                }
                // modify layer spec info
                feasiblePath->UpdateLayerSpecInfo(ingTSS, egrTSS, this->_userConstraint->getPreserveVlanAvailabilityRange());
                _feasiblePaths->push_back(feasiblePath);
                itP++;
            }
        }

        // restore TEWG by adding back conjoined delta
        for (itL = tewg->GetLinks().begin(); itL != tewg->GetLinks().end(); itL++)
        {
            TDelta* conjDelta = (TDelta*)(*itL)->GetWorkData()->GetData("CONJOINED_DELTA");
            // add to resource
            if (conjDelta != NULL)
                conjDelta->Revoke();
        }
        // break if feasiblePaths.size() == requested_num; Or get more paths then sort and return the best ones ?
    }
    if (_feasiblePaths->size() == 0)
    {
        delete _feasiblePaths;
        _feasiblePaths = NULL;
        throw ComputeThreadException((char*)"Action_ComputeSchedulesWithKSP::Process() No feasible path found after being applied with TE constraints!");
    }
    sort(_feasiblePaths->begin(), _feasiblePaths->end(), cmp_tpath);
    
    LOG_DEBUG("Action_ComputeSchedulesWithKSP::Process() found feasible paths for context=" << this->context << "path=" << this->_userConstraint->getPathId() <<endl);

    // Commit a selected (best) feasible path into the TEWG as concurrent constraint for subsequent paths in the same multi-P2P request
    if (this->yesCommitBestPathToTEWG())
    {
        TPath* committedPath = _feasiblePaths->front();
        TReservation* resv = new TReservation(_userConstraint->getGri());
        TSchedule* schedule = new TSchedule(committedPath->GetSchedules().front()->GetStartTime(), duration);
        resv->GetSchedules().push_back(schedule);
        TGraph* serviceTopo = new TGraph(_userConstraint->getGri());
        try {
            serviceTopo->LoadPath(committedPath->GetPath()); 
        } catch (TEDBException ex) {
            if (ex.GetMessage().find("has already existed") != string::npos) {
                throw ComputeThreadException((char*)"Action_ComputeSchedulesWithKSP::Process() found a path but the path is infeasible for provisioning!");
            } else {
                throw ComputeThreadException((char*)"Action_ComputeSchedulesWithKSP::Process() failed to commit a path!");
            }
        } 
        resv->SetServiceTopology(serviceTopo);
        string status = "RESERVED"; resv->SetStatus(status);
        resv->BuildDeltaCache();
        list<TReservation*>* committedResvations = (list<TReservation*>*)this->GetComputeWorker()->GetWorkflowData("COMMITTED_RESERVATIONS");
        if (committedResvations == NULL)
        {
            committedResvations = new list<TReservation*>;
            this->GetComputeWorker()->SetWorkflowData("COMMITTED_RESERVATIONS", (void*)committedResvations);
        }
        committedResvations->push_back(resv);
        list<TReservation*>::iterator itR = committedResvations->begin();
        for (; itR != committedResvations->end(); itR++)
            tewg->AddResvDeltas(*itR);
        tewg->RevokeResvDeltas(_userConstraint->getGri());
        tewg->ApplyResvDeltas(_userConstraint->getGri());
    }
}


bool Action_ComputeSchedulesWithKSP::ProcessChildren()
{
    LOG(name<<"ProcessChildren() called"<<endl);
    //$$$$ loop through all children to look for states

    // return true if all children have finished; otherwise false
    return Action::ProcessChildren();
}


bool Action_ComputeSchedulesWithKSP::ProcessMessages()
{
    LOG(name<<"ProcessMessages() called"<<endl);
    //$$$$ process messages if received
    //$$$$ run current action logic based on received messages 

    //return true if all messages received and processed; otherwise false
    return Action::ProcessMessages();
}


void Action_ComputeSchedulesWithKSP::CleanUp()
{
    LOG(name<<"CleanUp() called"<<endl);
    //$$$$ cleanup logic for current action
    // TODO: clean up link AggregateDeltaSeries in TEWG
    // TODO: this has to happen after BAG / RAF have been generated
    // TODO: clean up local _feasiblePaths
    // cancel and cleanup children
    Action::CleanUp();
}


void Action_ComputeSchedulesWithKSP::Finish()
{
    LOG(name<<"Finish() called"<<endl);
    //release mem for TEWG
    TEWG* tewg = (TEWG*)this->GetComputeWorker()->GetWorkflowData("TEWG");
    if (tewg != NULL)
        delete tewg;
    this->GetComputeWorker()->SetWorkflowData("TEWG", NULL);
    // stop out from event loop
    Action::Finish();
}


///////////////////// class Action_ProcessRequestTopology_MP2P  ///////////////////////////

// TODO: Verify TEWG is clean after each round of KSP processing

void Action_ProcessRequestTopology_MP2P::Process()
{
    LOG(name<<"Process() called"<<endl);   

    // set contextName
    if (this->GetComputeWorker()->GetWorkflowData("COMPUTE_CONTEXT") != NULL) 
        this->context = *(string*)this->GetComputeWorker()->GetWorkflowData("COMPUTE_CONTEXT");

    // retrieve userConstrinat list
    list<Apimsg_user_constraint*>* userConsList = (list<Apimsg_user_constraint*>*)this->GetComputeWorker()->GetWorkflowData("USER_CONSTRAINT_LIST");
    if (userConsList == NULL)
        throw ComputeThreadException((char*)"Action_ProcessRequestTopology_MP2P::Process() No USER_CONSTRAINT_LIST data from compute worker.");

    Apimsg_user_constraint* userConstraint = userConsList->front();
    u_int64_t volume = ((userConstraint->getFlexSchedules() && userConstraint->getFlexSchedules()->size() > 0)) ? userConstraint->getBandwidth()*userConstraint->getFlexSchedules()->front()->GetDuration() : 0;
    u_int64_t flexBandwidth = userConstraint->getBandwidth();
    if (volume == 0 || flexBandwidth == 0)
        throw ComputeThreadException((char*)"Action_ProcessRequestTopology_MP2P::Process() Zero bandwidth or transfer size. (Try simpleComputeWorker instead?)");
 
    // multiple sub-workflows for flexible requests
    vector<string> contextNameSet;
    contextNameSet.push_back("cxt_user_preferred_bw");
    contextNameSet.push_back("cxt_user_maximum_bw");
    contextNameSet.push_back("cxt_user_minimum_bw");
    for (int i = 0; i < contextNameSet.size(); i++)
    {
        // skip the context if first userConstraint (sub-path) does not have the corresponding flexible request
        // ?? should we go through the whole userConsList (all sub-paths) and then make the judgement? 
        if (contextNameSet[i]=="cxt_user_maximum_bw" && (userConstraint->getFlexMaxBandwidth() == 0 || userConstraint->getFlexMaxBandwidth() <= userConstraint->getBandwidth()))
            continue;
        else if (contextNameSet[i]=="cxt_user_minimum_bw" && (userConstraint->getFlexMinBandwidth() == 0 || userConstraint->getFlexMinBandwidth() >= userConstraint->getBandwidth()))
            continue;

        string actionName = "Action_CreateTEWG";
        Action_CreateTEWG* actionTewg = new Action_CreateTEWG(contextNameSet[i], actionName, this->GetComputeWorker());
        this->GetComputeWorker()->GetActions().push_back(actionTewg);
        // Each sub-workflow starts from common root (Action_ProcessRequestTopology_MP2P). 
        // They are parallel and thus can fail independently
        this->AddChild(actionTewg);

        // KSP w/ scheduling computation - first round (non-concurrent)
        list<Apimsg_user_constraint*>::iterator it = userConsList->begin();
        Action* prevAction = actionTewg;
        for (; it != userConsList->end(); it++)
        {
            userConstraint = *it;
            flexBandwidth = userConstraint->getBandwidth();
            volume = ((userConstraint->getFlexSchedules() && userConstraint->getFlexSchedules()->size() > 0)) ? userConstraint->getBandwidth()*userConstraint->getFlexSchedules()->front()->GetDuration() : 0;
            if (contextNameSet[i]=="cxt_user_maximum_bw")
            {
                if(volume > 0 && userConstraint->getFlexMaxBandwidth() > 0 && userConstraint->getFlexMaxBandwidth() > userConstraint->getBandwidth()) 
                    flexBandwidth = userConstraint->getFlexMaxBandwidth();
                else
                    continue;
            }
            else if (contextNameSet[i]=="cxt_user_minimum_bw")
            {
                if (volume > 0 && userConstraint->getFlexMinBandwidth() > 0 && userConstraint->getFlexMinBandwidth() < userConstraint->getBandwidth()) 
                    flexBandwidth = userConstraint->getFlexMinBandwidth();
                else
                    continue;
            } // else use userConstraint->getBandwidth and follow down

            actionName = "Action_ComputeKSP";
            Action_ComputeKSP* actionKsp = new Action_ComputeKSP(contextNameSet[i], actionName, this->GetComputeWorker());
            actionKsp->SetReqBandwidth(flexBandwidth);
            actionKsp->SetReqVolume(volume);
            actionKsp->SetComputeBAG(false);
            this->GetComputeWorker()->GetActions().push_back(actionKsp);
            prevAction->AddChild(actionKsp);
            
            actionName = "Action_CreateOrderedATS";
            Action_CreateOrderedATS* actionAts = new Action_CreateOrderedATS(contextNameSet[i], actionName, this->GetComputeWorker());
            actionAts->SetReqBandwidth(flexBandwidth);
            actionAts->SetReqVolume(volume);
            actionAts->SetUserConstraint(userConstraint);
            this->GetComputeWorker()->GetActions().push_back(actionAts);
            actionKsp->AddChild(actionAts);

            actionName = "Action_ComputeSchedulesWithKSP_Round1_";
            actionName += (*it)->getPathId();
            Action_ComputeSchedulesWithKSP* actionSchedKsp = new Action_ComputeSchedulesWithKSP(contextNameSet[i], actionName, this->GetComputeWorker());
            actionSchedKsp->SetReqBandwidth(flexBandwidth);
            actionSchedKsp->SetReqVolume(volume);
            actionSchedKsp->SetUserConstraint(*it);
            actionSchedKsp->SetComputeBAG(false);
            actionSchedKsp->SetCommitBestPathToTEWG(false);
            this->GetComputeWorker()->GetActions().push_back(actionSchedKsp);
            actionAts->AddChild(actionSchedKsp);
            prevAction = actionSchedKsp;
        }

        actionName = "Action_ReorderPaths_MP2P";
        Action* actionReorder = new Action_ReorderPaths_MP2P(contextNameSet[i], actionName, this->GetComputeWorker());
        this->GetComputeWorker()->GetActions().push_back(actionReorder);
        prevAction->AddChild(actionReorder);
    }
    
}


bool Action_ProcessRequestTopology_MP2P::ProcessChildren()
{
    LOG(name<<"ProcessChildren() called"<<endl);
    //$$$$ loop through all children to look for states

    // return true if all children have finished; otherwise false
    return Action::ProcessChildren();
}


bool Action_ProcessRequestTopology_MP2P::ProcessMessages()
{
    LOG(name<<"ProcessMessages() called"<<endl);
    //$$$$ process messages if received
    //$$$$ run current action logic based on received messages 

    //return true if all messages received and processed; otherwise false
    return Action::ProcessMessages();
}


void Action_ProcessRequestTopology_MP2P::CleanUp()
{
    LOG(name<<"CleanUp() called"<<endl);
    //$$$$ cleanup logic for current action
    
    // cancel and cleanup children
    Action::CleanUp();
}


void Action_ProcessRequestTopology_MP2P::Finish()
{
    LOG(name<<"Finish() called"<<endl);

    // collect results from exceptions (failuare) and  multiple Action_FinalizeServiceTopology_MP2P (success)
    string actionName = "Action_ComputeSchedulesWithKSP_Round2_";
    list<Apimsg_user_constraint*>* userConsList = (list<Apimsg_user_constraint*>*)this->GetComputeWorker()->GetWorkflowData("USER_CONSTRAINT_LIST");
    list<Apimsg_user_constraint*>::iterator itU = userConsList->begin();
    vector<string> contextNameSet;
    contextNameSet.push_back("cxt_user_preferred_bw");
    contextNameSet.push_back("cxt_user_maximum_bw");
    contextNameSet.push_back("cxt_user_minimum_bw");
    list<TLV*> tlvList;
    // combine results of multiple contexts by flexible request
    // retrieve COMPUTE_RESULT_LIST for all contexts
    actionName = "Action_FinalizeServiceTopology_MP2P";
    list<ComputeResult*>* computeResultList = (list<ComputeResult*>*)this->GetComputeWorker()->GetContextActionData(contextNameSet[0], actionName, "COMPUTE_RESULT_LIST");
    list<ComputeResult*>* computeResultList_max = (list<ComputeResult*>*)this->GetComputeWorker()->GetContextActionData(contextNameSet[1], actionName, "COMPUTE_RESULT_LIST");
    list<ComputeResult*>* computeResultList_min = (list<ComputeResult*>*)this->GetComputeWorker()->GetContextActionData(contextNameSet[2], actionName, "COMPUTE_RESULT_LIST");
    // for each result in list, append flexible (min, max) result to flexAlterPaths
    if (computeResultList != NULL && computeResultList->size() > 0)
    {
        list<ComputeResult*>::iterator itR, itR1, itR2;
        bool appendMax = false, appendMin = false;
        if (computeResultList_max != NULL && computeResultList_max->size() == computeResultList->size()) 
        {
            appendMax = true;
            itR1 = computeResultList_max->begin();
        }
        if (computeResultList_min != NULL && computeResultList_min->size() == computeResultList->size()) 
        {
            appendMin = true;
            itR2 = computeResultList_min->begin();
        }
        for (itR = computeResultList->begin(); itR != computeResultList->end(); itR++)
        {
            ComputeResult* result = *itR;
            if (appendMax)
            {
                result->GetFlexAlterPaths().push_back((*itR1)->GetPathInfo());
                itR1++;
            }
            if (appendMin)
            {
                result->GetFlexAlterPaths().push_back((*itR2)->GetPathInfo());
                itR2++;
            }
            TLV* tlv = (TLV*)new char[TLV_HEAD_SIZE + sizeof(void*)];
            tlv->type = MSG_TLV_VOID_PTR;
            tlv->length = sizeof(void*);
            memcpy(tlv->value, &result, sizeof(void*));
            tlvList.push_back(tlv);
        }
    }
    // failure case
    if (tlvList.size() == 0)
    {
        ComputeResult* result = new ComputeResult(userConsList->front()->getGri());
        string errMsg = "Action_ProcessRequestTopology_MP2P::Finish() Cannot find the set of paths for the RequestTopology.";
        result->SetErrMessage(errMsg);
        TLV* tlv = (TLV*)new char[TLV_HEAD_SIZE + sizeof(void*)];
        tlv->type = MSG_TLV_VOID_PTR;
        tlv->length = sizeof(void*);
        memcpy(tlv->value, &result, sizeof(void*));
        tlvList.push_back(tlv);
    }
    // TODO: clean up multip2pResultList
    string queue = MxTCE::computeThreadPrefix + worker->GetName();
    string topic = "COMPUTE_REPLY";
    SendMessage(MSG_REPLY, queue, topic, this->context, tlvList);

    // stop out from event loop
    Action::Finish();
}




///////////////////// class Action_MP2P_ReorderPaths_MP2P  ///////////////////////////

inline time_t Action_ReorderPaths_MP2P::OverlappingTime(time_t st1, time_t et1, time_t st2, time_t et2)
{
    if (st1 == 0 && st2 == 0)
        return MAX_SCHEDULE_DURATION;
    else if (st1 != 0 && st2 == 0)
        return (et1 - st1) > MAX_SCHEDULE_DURATION ? MAX_SCHEDULE_DURATION : et1 - st1;
    else if (st1 == 0 && st2 != 0)
        return (et2 - st2) > MAX_SCHEDULE_DURATION ? MAX_SCHEDULE_DURATION : et2 - st2;
    //else
    time_t duration = 0;
    if (st1 <= st2 && et1 <= et2)
        duration = et1 - st2;
    else if (st1 >= st2 && et1 <= et2)
       duration = et1 - st1;
    else if (st1 <= st2 && et1 >= et2)
       duration = et2 - st2;
    else
       duration = et2 - st1;
    //return
    if (duration <= 0)
       return 0;
    if (duration > MAX_SCHEDULE_DURATION)
       return MAX_SCHEDULE_DURATION;
    return duration;
}


inline time_t Action_ReorderPaths_MP2P::GetPathOverlappingTime(TPath* path1, TPath* path2)
{
    list<TSchedule*>::iterator itS1 = path1->GetSchedules().begin();
    list<TSchedule*>::iterator itS2 = path2->GetSchedules().begin();
    time_t total = 0;
    for (; itS1 != path1->GetSchedules().end(); itS1 ++) 
    {
        for (; itS2 != path2->GetSchedules().end(); itS2 ++) 
        {
            total += OverlappingTime((*itS1)->GetStartTime(), (*itS1)->GetEndTime(), (*itS2)->GetStartTime(), (*itS2)->GetEndTime());
        }
    }
    return total;
}


inline double Action_ReorderPaths_MP2P::BandwidthWeightedHopLength(TPath* P)
{
    return P->GetPath().front()->GetMaxBandwidth()*P->GetPath().size();
}

inline double Action_ReorderPaths_MP2P::SumOfBandwidthTimeWeightedCommonLinks(TPath* P, vector<TPath*>& Paths)
{
    if (P->GetPath().size() == 0)
        return 0;
    int i, numPaths = Paths.size();

    for (i = 0; i < numPaths; i++)
    {
        if (*P == *Paths[i])
            break;
    }
    //assert (i < numPaths);
    if (i == numPaths) return 0;
    TPath* path1 = Paths[i];

    double sum = 0;
    list<TLink*>::iterator iter1, iter2;
    for (iter1 = path1->GetPath().begin(); iter1 != path1->GetPath().end(); iter1++)
    {
        for (i = 0; i < numPaths; i++)
        {
            if (path1== Paths[i])
                continue;
            double overlap = GetPathOverlappingTime(path1, Paths[i]);
            for (iter2 = Paths[i]->GetPath().begin(); iter2 != Paths[i]->GetPath().end(); iter2++)
            {
                if ((*iter1) == (*iter2))
                {
                    u_int64_t bw1 = (*iter1)->GetMaxBandwidth();
                    u_int64_t bw2 = (*iter2)->GetMaxBandwidth();
                    sum += (bw1<bw2?bw1:bw2)*(1.0-BANDWIDTH_TIME_FACTOR+BANDWIDTH_TIME_FACTOR*(double)overlap/(double)MAX_SCHEDULE_DURATION);
                }
            }
        }
    }
    return sum;
}

inline void Action_ReorderPaths_MP2P::Swap(Action_ComputeSchedulesWithKSP* &ksp_i, Action_ComputeSchedulesWithKSP* &ksp_j)
{
    Action_ComputeSchedulesWithKSP* p;
    p = ksp_i;
    ksp_i = ksp_j;
    ksp_j = p;
}


void Action_ReorderPaths_MP2P::Process()
{
    LOG(name<<"Process() called"<<endl);

    string actionName;
    list<Apimsg_user_constraint*>* userConsList = (list<Apimsg_user_constraint*>*)this->GetComputeWorker()->GetWorkflowData("USER_CONSTRAINT_LIST");

    // find first-round KSP actions
    vector<Action_ComputeSchedulesWithKSP*> round1KspActions;
    vector<TPath*> path1All;
    list<Apimsg_user_constraint*>::iterator itU = userConsList->begin();
    for (itU = userConsList->begin(); itU != userConsList->end(); itU++)
    {
        actionName = "Action_ComputeSchedulesWithKSP_Round1_";
        actionName += (*itU)->getPathId();
        Action_ComputeSchedulesWithKSP* actionSchedKspR1 = (Action_ComputeSchedulesWithKSP*)this->GetComputeWorker()->LookupAction(context, actionName);
        assert(actionSchedKspR1);
        round1KspActions.push_back(actionSchedKspR1);
        string dataName = "FEASIBLE_PATHS";
        path1All.push_back(((vector<TPath*>*)actionSchedKspR1->GetData(dataName))->front());
    }
    
    // re-order the first-round KSP actions
    for (int i = 0; i < round1KspActions.size(); i++)
    {
        for (int j = 0; j < round1KspActions.size(); j++)
        {
            if (j > i)
            {
                Action_ComputeSchedulesWithKSP *ksp_i = round1KspActions[i], *ksp_j = round1KspActions[j];
                string dataName = "FEASIBLE_PATHS";
                TPath* path1_i = ((vector<TPath*>*)ksp_i->GetData(dataName))->front();
                TPath* path1_j = ((vector<TPath*>*)ksp_j->GetData(dataName))->front();
                if (path1_i == NULL || path1_j == NULL)
                    continue;
                if (BandwidthWeightedHopLength(path1_i) < BandwidthWeightedHopLength(path1_j))
                    Swap(ksp_i, ksp_j);
                else if (SumOfBandwidthTimeWeightedCommonLinks(path1_i, path1All) < SumOfBandwidthTimeWeightedCommonLinks(path1_j, path1All))
                    Swap(ksp_i, ksp_j);
            }
        }
    }

    // array round1KspActions has been reordered
    // run KSP w/ scheduling computation - second round (concurrent after reordering )
    vector<Action_ComputeSchedulesWithKSP*>::iterator itK;
    Action* prevAction = this;
    for (itK = round1KspActions.begin(); itK != round1KspActions.end(); itK++)
    {
        Action_ComputeSchedulesWithKSP* actionSchedKspR1 = *itK;
        actionName = "Action_ComputeSchedulesWithKSP_Round2_";
        actionName += actionSchedKspR1->GetUserConstraint()->getPathId();
        Action_ComputeSchedulesWithKSP* actionSchedKspR2 = new Action_ComputeSchedulesWithKSP(this->context, actionName, this->GetComputeWorker());
        actionSchedKspR2->SetReqBandwidth(actionSchedKspR1->GetReqBandwidth());
        actionSchedKspR2->SetReqVolume(actionSchedKspR1->GetReqVolume());
        actionSchedKspR2->SetUserConstraint(actionSchedKspR1->GetUserConstraint());
        actionSchedKspR2->SetComputeBAG(true);
        actionSchedKspR2->SetCommitBestPathToTEWG(MxTCE::exclusiveConcurrentHolding);
        this->GetComputeWorker()->GetActions().push_back(actionSchedKspR2);
        prevAction->AddChild(actionSchedKspR2);
        prevAction = actionSchedKspR2;
    }
    
    // each Action_FinalizeServiceTopology_MP2P handle result for one sub-workflow
    actionName = "Action_FinalizeServiceTopology_MP2P";
    Action_FinalizeServiceTopology_MP2P* actionFinal = new Action_FinalizeServiceTopology_MP2P(this->context, actionName, this->GetComputeWorker());
    this->GetComputeWorker()->GetActions().push_back(actionFinal);
    prevAction->AddChild(actionFinal);

}


bool Action_ReorderPaths_MP2P::ProcessChildren()
{
    LOG(name<<"ProcessChildren() called"<<endl);
    //$$$$ loop through all children to look for states

    // return true if all children have finished; otherwise false
    return Action::ProcessChildren();
}


bool Action_ReorderPaths_MP2P::ProcessMessages()
{
    LOG(name<<"ProcessMessages() called"<<endl);
    //$$$$ process messages if received
    //$$$$ run current action logic based on received messages 

    //return true if all messages received and processed; otherwise false
    return Action::ProcessMessages();
}


void Action_ReorderPaths_MP2P::CleanUp()
{
    LOG(name<<"CleanUp() called"<<endl);
    //$$$$ cleanup logic for current action

    // cancel and cleanup children
    Action::CleanUp();
}


void Action_ReorderPaths_MP2P::Finish()
{
    LOG(name<<"Finish() called"<<endl);

    //$$$$ finish logic for current action

    // stop out from event loop
    Action::Finish();
}


///////////////////// class Action_MP2P_FinalizeServiceTopology_MP2P ///////////////////////////


void* Action_FinalizeServiceTopology_MP2P::GetData(string& dataName)
{
    if (dataName == "COMPUTE_RESULT_LIST")
        return _computeResultList;
    return NULL;
}


// normalize result for multi-p2p of one sub-workflow (fixed bw)
void Action_FinalizeServiceTopology_MP2P::Process()
{
    LOG(name<<"Process() called"<<endl);
    
    // compose MP2P ComputeResult data
    _computeResultList = new list<ComputeResult*>;

    // collect feasible paths for all p2p sub-flows
    list<Apimsg_user_constraint*>* userConsList = (list<Apimsg_user_constraint*>*)this->GetComputeWorker()->GetWorkflowData("USER_CONSTRAINT_LIST");
    list<Apimsg_user_constraint*>::iterator it = userConsList->begin();
    for (; it != userConsList->end(); it++)
    {   
        Apimsg_user_constraint* userConstraint = *it;
        string actionName = "Action_ComputeSchedulesWithKSP_Round2_";
        actionName += userConstraint->getPathId();
        vector<TPath*>* feasiblePaths = (vector<TPath*>*)this->GetComputeWorker()->GetContextActionData(this->context, actionName, "FEASIBLE_PATHS");
        ComputeResult* result = new ComputeResult(userConstraint->getGri());
        result->SetPathId(userConstraint->getPathId());
        _computeResultList->push_back(result);
        if (feasiblePaths && feasiblePaths->size() > 0)
        {
            TPath* resultPath = feasiblePaths->front()->Clone();
            resultPath->LogDump();
            resultPath->SetIndependent(true); 
            ComputeResult::RegulatePathInfo(resultPath);
            result->SetPathInfo(resultPath);
            if (userConstraint->getCoschedreq()&& feasiblePaths->size() > 1) 
            {
                result->GetAlterPaths().clear();
                for (int k = 1; k < feasiblePaths->size() && k < userConstraint->getCoschedreq()->getMaxnumofaltpaths(); k++)
                {
                    resultPath = (*feasiblePaths)[k]->Clone();
                    resultPath->LogDump();
                    resultPath->SetIndependent(true); 
                    ComputeResult::RegulatePathInfo(resultPath);
                    result->GetAlterPaths().push_back(resultPath);
                }
            }
        }
        else
        {
            char buf[1024];
            snprintf(buf, 1024, "Action_FinalizeServiceTopology_MP2P::Process() No feasible path found for GRI: %s, Path: %s under Context: %s!", 
                userConstraint->getGri().c_str(), userConstraint->getPathId().c_str(), this->context.c_str());
            LOG(buf << endl);
            // TODO: set conext-action error msg 
            break;
        }
    }
}


bool Action_FinalizeServiceTopology_MP2P::ProcessChildren()
{
    LOG(name<<"ProcessChildren() called"<<endl);
    //$$$$ loop through all children to look for states

    // return true if all children have finished; otherwise false
    return Action::ProcessChildren();
}


bool Action_FinalizeServiceTopology_MP2P::ProcessMessages()
{
    LOG(name<<"ProcessMessages() called"<<endl);
    //$$$$ process messages if received
    //$$$$ run current action logic based on received messages 

    //return true if all messages received and processed; otherwise false
    return Action::ProcessMessages();
}


void Action_FinalizeServiceTopology_MP2P::CleanUp()
{
    LOG(name<<"CleanUp() called"<<endl);
    //$$$$ cleanup logic for current action

    // cancel and cleanup children
    Action::CleanUp();
}


void Action_FinalizeServiceTopology_MP2P::Finish()
{
    LOG(name<<"Finish() called"<<endl);

    //$$$$ finish logic for current action

    // stop out from event loop
    Action::Finish();
}

