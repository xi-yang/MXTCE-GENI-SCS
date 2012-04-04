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

    string paramName = "USER_CONSTRAINT";
    Apimsg_user_constraint* userConstraint = (Apimsg_user_constraint*)this->GetComputeWorker()->GetParameter(paramName);
    ComputeResult* result = new ComputeResult(userConstraint->getGri());

    paramName = "KSP";
    vector<TPath*>* KSP = (vector<TPath*>*)this->GetComputeWorker()->GetParameter(paramName);
    paramName = "FEASIBLE_PATHS";
    vector<TPath*>* feasiblePaths = (vector<TPath*>*)this->GetComputeWorker()->GetParameter(paramName);
    if (feasiblePaths == NULL)
        feasiblePaths = KSP;

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
        paramName = "ERROR_MSG";
        result->SetErrMessage(*(string*)(this->GetComputeWorker()->GetParameter(paramName)));
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

    // destroy stored data including tewg
    if (KSP)
    {
        delete KSP;
        this->GetComputeWorker()->SetParameter(paramName, NULL);
    }
    paramName = "TEWG";
    TEWG* tewg = (TEWG*)this->GetComputeWorker()->GetParameter(paramName);
    if (tewg)
    {
        delete tewg;
        this->GetComputeWorker()->SetParameter(paramName, NULL);
    }

    // destroy feasiblePaths ? 

    // stop out from event loop
    Action::Finish();
}



///////////////////// class Action_CreateTEWG ///////////////////////////

void Action_CreateTEWG::Process()
{
    LOG(name<<"Process() called"<<endl);

    // run current action main logic
    string queue = MxTCE::computeThreadPrefix + worker->GetName();
    string topic = "TEWG_REQUEST";
    string expectReturnTopic = "TEWG_REPLY";
    list<TLV*> noTLVs;
    SendMessage(MSG_REQ, queue, topic, noTLVs, expectReturnTopic);
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
            TEWG* tewg; 
            memcpy(&tewg, (TEWG*)tlvList.front()->value, sizeof(void*));
            // store TEWG
            string paramName = "TEWG";
            this->GetComputeWorker()->SetParameter(paramName, (void*)tewg);
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

void Action_ComputeKSP::Process()
{
    LOG(name<<"Process() called"<<endl);
    string paramName = "TEWG";
    TEWG* tewg = (TEWG*)this->GetComputeWorker()->GetParameter(paramName);
    if (tewg == NULL)
        throw ComputeThreadException((char*)"Action_ComputeKSP::Process() No TEWG available for computation!");

    //  the user request parameters
    paramName = "USER_CONSTRAINT";
    Apimsg_user_constraint* userConstraint = (Apimsg_user_constraint*)this->GetComputeWorker()->GetParameter(paramName);
    TNode* srcNode = tewg->LookupNodeByURN(userConstraint->getSrcendpoint());
    if (srcNode == NULL)
        throw ComputeThreadException((char*)"Action_ComputeKSP::Process() unknown source URN!");
    TNode* dstNode = tewg->LookupNodeByURN(userConstraint->getDestendpoint());
    if (dstNode == NULL)
        throw ComputeThreadException((char*)"Action_ComputeKSP::Process() unknown destination URN!");
    u_int64_t bw = (u_int64_t)userConstraint->getBandwidth();
    if (userConstraint->getCoschedreq()&& userConstraint->getCoschedreq()->getMinbandwidth() > bw)
        bw = userConstraint->getCoschedreq()->getMinbandwidth();
    u_int32_t srcVtag, dstVtag;
    if (userConstraint->getSrcvlantag() == "any" || userConstraint->getSrcvlantag() == "ANY")
        srcVtag = ANY_TAG;
    else
        sscanf(userConstraint->getSrcvlantag().c_str(), "%d", &srcVtag);
    if (userConstraint->getDestvlantag() == "any" || userConstraint->getDestvlantag() == "ANY")
        dstVtag = ANY_TAG;
    else
        sscanf(userConstraint->getDestvlantag().c_str(), "%d", &dstVtag);

    TSpec tspec;
    if (userConstraint->getLayer() == "3")
        tspec.Update(LINK_IFSWCAP_PSC1, LINK_IFSWCAP_ENC_PKT, bw);
    else
        tspec.Update(LINK_IFSWCAP_L2SC, LINK_IFSWCAP_ENC_ETH, bw);

    // reservations pruning
    // for current OSCARS implementation, tcePCE should pass startTime==endTime==0
    time_t startTime = userConstraint->getStarttime();
    time_t endTime = userConstraint->getEndtime();
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

    // prune bandwidth
    tewg->LogDump();
    tewg->PruneByBandwidth(bw);

    TLink* ingressLink = tewg->LookupLinkByURN(userConstraint->getSrcendpoint());
    if (!ingressLink || !ingressLink->IsAvailableForTspec(tspec))
        throw ComputeThreadException((char*)"Action_ComputeKSP::Process() Ingress Edge Link is not available for requested TSpec!");
    TLink* egressLink = tewg->LookupLinkByURN(userConstraint->getDestendpoint());
    if (!egressLink || !egressLink->IsAvailableForTspec(tspec))
        throw ComputeThreadException((char*)"Action_ComputeKSP::Process() Egress Edge Link is not available for requested TSpec!");

    // compute KSP
    vector<TPath*>* KSP = new vector<TPath*>;
    try {
        tewg->ComputeKShortestPaths(srcNode, dstNode, tewg->GetNodes().size()*2>20?20:tewg->GetNodes().size()*2, *KSP);
    } catch (TCEException e) {
        LOG_DEBUG("Action_ComputeKSP::Process raised exception: " << e.GetMessage() <<endl);
        throw ComputeThreadException(e.GetMessage());
    }

    paramName = "KSP";
    if (KSP->size() == 0)
    {
        delete KSP;
        this->GetComputeWorker()->SetParameter(paramName, NULL);
        LOG_DEBUG("Action_ComputeKSP::Process() No KSP found after bandwidh pruning!" <<endl);
        throw ComputeThreadException((char*)"Action_ComputeKSP::Process() No KSP found after bandwidh pruning!");
    }
    this->GetComputeWorker()->SetParameter(paramName, KSP);

    // verify constraints with switchingType / layer adaptation / VLAN etc.
    vector<TPath*>::iterator itP = KSP->begin(); 
    while (itP != KSP->end())
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

        // TODO: need special handling for old OSCARS L2SC --> PSC edge adaptation: add artificial IACD for  (ingress <-> 1st-hop and egress <-> last-hop)

        TServiceSpec ingTSS, egrTSS;
        ingTSS.Update(tspec.SWtype, tspec.ENCtype, tspec.Bandwidth);
        ingTSS.GetVlanSet().AddTag(srcVtag);
        egrTSS.Update(tspec.SWtype, tspec.ENCtype, tspec.Bandwidth);
        egrTSS.GetVlanSet().AddTag(dstVtag);
        (*itP)->ExpandWithRemoteLinks();
        if (!(*itP)->VerifyTEConstraints(ingTSS, egrTSS))
        {
            TPath* path2erase = *itP;
            itP = KSP->erase(itP);
            delete path2erase;
        }
        else
        {
            paramName = "FEASIBLE_PATHS";
            vector<TPath*>* feasiblePaths = (vector<TPath*>*)this->GetComputeWorker()->GetParameter(paramName);
            if (feasiblePaths == NULL)
            {
                feasiblePaths = new vector<TPath*>;
                this->GetComputeWorker()->SetParameter(paramName, feasiblePaths);
            }
			// make a copy of TPath from work set. Then do twists on the copy to satisfy reply format.
			// Caution: Clone() will inherit the orignal localEnd and remoteEnd nodes from work set.
            TPath* feasiblePath = (*itP)->Clone();
            // check whether BAG is requested
            if (userConstraint->getCoschedreq() && userConstraint->getCoschedreq()->getBandwidthavaigraph()) 
            {
                BandwidthAvailabilityGraph* bag = (*itP)->CreatePathBAG(userConstraint->getCoschedreq()->getStarttime(), 
                    userConstraint->getCoschedreq()->getEndtime());
                if (bag != NULL) 
                {
                    feasiblePath->SetBAG(bag);
                    (*itP)->SetBAG(NULL);
                }
            }
            // modify bandwidth to service bw
            for (itL = feasiblePath->GetPath().begin(); itL != feasiblePath->GetPath().end(); itL++)
                (*itL)->SetMaxBandwidth(bw);
            // modify layer spec info
            feasiblePath->UpdateLayerSpecInfo(ingTSS, egrTSS);
            feasiblePaths->push_back(feasiblePath);
            itP++;
        }
    }
    // store a list of ordered result paths 
    if (KSP->size() == 0)
    {
        delete KSP;
        this->GetComputeWorker()->SetParameter(paramName, NULL);
        throw ComputeThreadException((char*)"Action_ComputeKSP::Process() No KSP found after being applied with TE constraints!");
    }
    // debugging output
    sort(KSP->begin(), KSP->end(), cmp_tpath);
    for (itP = KSP->begin(); itP != KSP->end(); itP++)
    {
        (*itP)->LogDump();
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

    string paramName = "FEASIBLE_PATHS";

    vector<TPath*>* feasiblePaths = (vector<TPath*>*)this->GetComputeWorker()->GetParameter(paramName);

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
        paramName = "KSP";
        vector<TPath*>* KSP = (vector<TPath*>*)this->GetComputeWorker()->GetParameter(paramName);

        // TODO: pick one or multiple paths (or return failure)
        // TODO: combine with feasible paths above?
        if (KSP == NULL || KSP->size() == 0)
            throw ComputeThreadException((char*)"Action_FinalizeServiceTopology::Process() No path found!");
        
        // TODO:  translate into format API requires
        (*min_element(KSP->begin(), KSP->end(), cmp_tpath))->LogDump();
        //$$ generate path BAG too ?
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

inline void Action_CreateOrderedATS::AddUniqueTimePoint(vector<time_t>* ats, time_t t)
{
    vector<time_t>::iterator it;
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
    
    string paramName = "KSP";
    vector<TPath*>* KSP = (vector<TPath*>*)this->GetComputeWorker()->GetParameter(paramName);

    if (KSP == NULL || KSP->size() == 0)
        throw ComputeThreadException((char*)"Action_CreateOrderedATS::Process() Empty KSP list: no path found!");

    paramName = "TEWG";
    TEWG* tewg = (TEWG*)this->GetComputeWorker()->GetParameter(paramName);

    vector<TPath*>::iterator itP;
    list<TLink*>::iterator itL;
    int* piVal = new int(0);
    for (itL = tewg->GetLinks().begin(); itL != tewg->GetLinks().end(); itL++)
        (*itL)->GetWorkData()->SetData("ATS_Order_Counter", (void*)piVal);
    for (itP = KSP->begin(); itP != KSP->end(); itP++)
    {
        TPath* P = *itP;
        for (itL = P->GetPath().begin(); itL != P->GetPath().end(); itL++)
        {
            piVal = new int ((*itL)->GetWorkData()->GetInt("ATS_Order_Counter"));
            (*piVal)++;
            (*itL)->GetWorkData()->SetData("ATS_Order_Counter", piVal);
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

    _orderedATS = new vector<time_t>;
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

    // workflow data
    string paramName = "TEWG";
    TEWG* tewg = (TEWG*)this->GetComputeWorker()->GetParameter(paramName);
    if (tewg == NULL)
        throw ComputeThreadException((char*)"Action_ComputeSchedulesWithKSP::Process() No TEWG available for computation!");
    paramName = "USER_CONSTRAINT";
    if (_userConstraint == NULL)
        _userConstraint = (Apimsg_user_constraint*)this->GetComputeWorker()->GetParameter(paramName);

    // context data
    string actionName = "Action_CreateOrderedATS";
    string dataName = "ORDERED_ATS";
    vector<time_t>* orderedATS = (vector<time_t>*)this->GetComputeWorker()->GetContextData(this->context, actionName, dataName);
    if (tewg == NULL)
        throw ComputeThreadException((char*)"Action_ComputeSchedulesWithKSP::Process() No Ordered Aggregate Time Series available for computation!");

    TNode* srcNode = tewg->LookupNodeByURN(_userConstraint->getSrcendpoint());
    if (srcNode == NULL)
        throw ComputeThreadException((char*)"Action_ComputeSchedulesWithKSP::Process() unknown source URN!");
    TNode* dstNode = tewg->LookupNodeByURN(_userConstraint->getDestendpoint());
    if (dstNode == NULL)
        throw ComputeThreadException((char*)"Action_ComputeSchedulesWithKSP::Process() unknown destination URN!");
    u_int64_t bw = (this->GetReqBandwidth() == 0 ? _userConstraint->getBandwidth():this->GetReqBandwidth());
    int duration = (this->GetReqVolume() == 0 ? _userConstraint->getEndtime()-_userConstraint->getStarttime():this->GetReqVolume()/this->GetReqBandwidth());
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
    for (int i = 0; i < orderedATS->size(); i++)
    {
        time_t startTime = (*orderedATS)[i];
        time_t endTime = startTime + duration;
        list<TLink*>::iterator itL;
        for (itL = tewg->GetLinks().begin(); itL != tewg->GetLinks().end(); itL++)
        {
            // $$ get conjoined ADS delta list in window
            AggregateDeltaSeries* ads = (AggregateDeltaSeries*)((*itL)->GetWorkData()->GetData("ADS"));
            TDelta* conjDelta = ads->JoinADSInWindow(startTime, endTime);
            (*itL)->GetWorkData()->SetData("CONJOINED_DELTA", conjDelta);
            // $$ deduct resource by conjoined delta
            conjDelta->SetTargetResource(*itL);
            conjDelta->Apply();
        }
        // $$ pruning bandwidth
        tewg->PruneByBandwidth(this->GetReqBandwidth());
        
        TLink* ingressLink = tewg->LookupLinkByURN(_userConstraint->getSrcendpoint());
        if (!ingressLink || !ingressLink->IsAvailableForTspec(tspec))
            throw ComputeThreadException((char*)"Action_ComputeKSP::Process() Ingress Edge Link is not available for requested TSpec!");
        TLink* egressLink = tewg->LookupLinkByURN(_userConstraint->getDestendpoint());
        if (!egressLink || !egressLink->IsAvailableForTspec(tspec))
            throw ComputeThreadException((char*)"Action_ComputeKSP::Process() Egress Edge Link is not available for requested TSpec!");
        
        // compute KSP
        KSP.clear();
        try {
            tewg->ComputeKShortestPaths(srcNode, dstNode, tewg->GetNodes().size()*2>20?20:tewg->GetNodes().size()*2, KSP);
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
            if (!(*itP)->VerifyTEConstraints(ingTSS, egrTSS))
            {
                TPath* path2erase = *itP;
                itP = KSP.erase(itP);
                delete path2erase;
            }
            else
            {
                TSchedule* schedule = new TSchedule(startTime, endTime);
                vector<TPath*>::iterator itFP = _feasiblePaths->begin();
                for (; itFP != _feasiblePaths->end(); itFP++)
                {
                    if ((*(*itFP)) == (*(*itP)))
                    {
                        (*itFP)->GetSchedules().push_back(schedule);
                        itP++;
                        continue;
                    }
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
                }
                feasiblePath->GetSchedules().push_back(schedule);
                // modify bandwidth to service bw
                for (itL = feasiblePath->GetPath().begin(); itL != feasiblePath->GetPath().end(); itL++)
                    (*itL)->SetMaxBandwidth(bw);
                // modify layer spec info
                feasiblePath->UpdateLayerSpecInfo(ingTSS, egrTSS);
                _feasiblePaths->push_back(feasiblePath);
                itP++;
            }
        }

        // $$ resore TEWG by adding back conjoined delta
        for (itL = tewg->GetLinks().begin(); itL != tewg->GetLinks().end(); itL++)
        {
            TDelta* conjDelta = (TDelta*)(*itL)->GetWorkData()->GetData("CONJOINED_DELTA");
            // $$ add to resource
            if (conjDelta != NULL)
                conjDelta->Revoke();
        }
        // $$ break if feasiblePaths.size() == requested_num; Or get more paths then sort and return the best ones ?
    }
    if (_feasiblePaths->size() == 0)
    {
        delete _feasiblePaths;
        _feasiblePaths = NULL;
        throw ComputeThreadException((char*)"Action_ComputeSchedulesWithKSP::Process() No feasible path found after being applied with TE constraints!");
    }
    sort(_feasiblePaths->begin(), _feasiblePaths->end(), cmp_tpath);
    // $$ commit a selected (best) feasible path into the TEWG as constraint for multi-P2P request
    if (this->yesCommitBestPathToTEWG())
    {
        TPath* committedPath = _feasiblePaths->front();
        TReservation* resv = new TReservation(_userConstraint->getGri());
        TSchedule* schedule = new TSchedule(committedPath->GetSchedules().front()->GetStartTime(), duration);
        resv->GetSchedules().push_back(schedule);
        TGraph* serviceTopo = new TGraph(_userConstraint->getGri());
        serviceTopo->LoadPath(committedPath->GetPath()); 
        resv->SetServiceTopology(serviceTopo);
        string status = "RESERVED"; resv->SetStatus(status);
        resv->BuildDeltaCache();
        paramName = "COMMITTED_RESERVATIONS"; // workflow level data
        list<TReservation*>* committedResvations = (list<TReservation*>*)this->GetComputeWorker()->GetParameter(paramName);
        if (committedResvations == NULL)
        {
            committedResvations = new list<TReservation*>;
            this->GetComputeWorker()->SetParameter(paramName, (void*)committedResvations);
        }
        committedResvations->push_back(resv);
        list<TReservation*>::iterator itR = committedResvations->begin();
        for (; itR != committedResvations->end(); itR++)
            tewg->AddResvDeltas(*itR);
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
    //$$$$ finish logic for current action

    // stop out from event loop
    Action::Finish();
}


///////////////////// class Action_ProcessRequestTopology_MP2P  ///////////////////////////

void Action_ProcessRequestTopology_MP2P::Process()
{
    LOG(name<<"Process() called"<<endl);   
    //$$ retrieve userConstrinat list

    string paramName = "USER_CONSTRAINT_LIST";
    list<Apimsg_user_constraint*>* userConsList = (list<Apimsg_user_constraint*>*)this->GetComputeWorker()->GetParameter(paramName);
    if (userConsList)
        throw ComputeThreadException((char*)"Action_ProcessRequestTopology_MP2P::Process() No USER_CONSTRAINT_LIST data from compute worker.");

    Apimsg_user_constraint* userConstraint = userConsList->front(); // assume all p2p paths have same flexible requirement
    u_int64_t volume = (userConstraint->getFlexSchedules().size() > 0) ? userConstraint->getBandwidth()*userConstraint->getFlexSchedules().front()->GetDuration() : 0;
    vector<u_int64_t> flexBandwidthSet;
    vector<string> contextNameSet;
    flexBandwidthSet.push_back(userConstraint->getBandwidth());
    contextNameSet.push_back("cxt_user_preferred_bw");
    if (volume > 0 && userConstraint->getFlexMaxBandwidth() > 0 && userConstraint->getFlexMaxBandwidth() != userConstraint->getBandwidth()) 
    {
        flexBandwidthSet.push_back(userConstraint->getFlexMaxBandwidth());
        contextNameSet.push_back("cxt_user_maximum_bw");
    }    
    if (volume > 0 && userConstraint->getFlexMinBandwidth() > 0 && userConstraint->getFlexMinBandwidth() != userConstraint->getBandwidth()) 
    {
        flexBandwidthSet.push_back(userConstraint->getFlexMinBandwidth());
        contextNameSet.push_back("cxt_user_minimum_bw");
    }

    // multi-flows for flexible requests
    string actionName = "Action_CreateTEWG";
    for (int i = 0; i < flexBandwidthSet.size(); i++)
    {
        Action_CreateTEWG* actionTewg = new Action_CreateTEWG(actionName, this->GetComputeWorker());
        this->GetComputeWorker()->GetActions().push_back(actionTewg);
        this->AddChild(actionTewg);
        
        actionName = "Action_CreateOrderedATS";
        Action_CreateOrderedATS* actionAts = new Action_CreateOrderedATS(contextNameSet[i], actionName, this->GetComputeWorker());
        actionAts->SetReqBandwidth(flexBandwidthSet[i]);
        actionAts->SetReqVolume(volume);
        this->GetComputeWorker()->GetActions().push_back(actionAts);
        actionTewg->AddChild(actionAts);

        // KSP w/ scheduling computation - first round (non-concurrent)
        list<Apimsg_user_constraint*>::iterator it = userConsList->begin();
        Action* prevAction = actionAts;
        for (; it != userConsList->end(); it++)
        {
            actionName = "Action_ComputeSchedulesWithKSP";
            actionName += "_Round1_";
            actionName += (*it)->getPathId();
            Action_ComputeSchedulesWithKSP* actionKsp = new Action_ComputeSchedulesWithKSP(contextNameSet[i], actionName, this->GetComputeWorker());
            actionKsp->SetReqBandwidth(flexBandwidthSet[i]);
            actionKsp->SetReqVolume(volume);
            actionKsp->SetUserConstraint(*it);
            actionKsp->SetComputeBAG(false);
            actionKsp->SetCommitBestPathToTEWG(false);
            this->GetComputeWorker()->GetActions().push_back(actionKsp);
            prevAction->AddChild(actionKsp);
            prevAction = actionKsp;
        }

        actionName = "Action_ReorderPaths_MP2P";
        Action* actionReorder = new Action_ReorderPaths_MP2P(contextNameSet[i], actionName, this->GetComputeWorker());
        this->GetComputeWorker()->GetActions().push_back(actionReorder);
        prevAction->AddChild(actionReorder);

        // KSP w/ scheduling computation - second round (concurrent)
        prevAction = actionReorder;
        for (it = userConsList->begin(); it != userConsList->end(); it++)
        {
            actionName = "Action_ComputeSchedulesWithKSP";
            actionName += "_Round2_";
            actionName += (*it)->getPathId();
            Action_ComputeSchedulesWithKSP* actionKsp = new Action_ComputeSchedulesWithKSP(contextNameSet[i], actionName, this->GetComputeWorker());
            actionKsp->SetReqBandwidth(flexBandwidthSet[i]);
            actionKsp->SetReqVolume(volume);
            actionKsp->SetUserConstraint(*it);
            actionKsp->SetComputeBAG(true);
            actionKsp->SetCommitBestPathToTEWG(true);
            this->GetComputeWorker()->GetActions().push_back(actionKsp);
            prevAction->AddChild(actionKsp);
            prevAction = actionKsp;
        }

        // each Action_FinalizeServiceTopology_MP2P handle result for one sub-workflow
        actionName = "Action_FinalizeServiceTopology_MP2P_";
        actionName += (*it)->getPathId();
        Action_FinalizeServiceTopology_MP2P* actionFinal = new Action_FinalizeServiceTopology_MP2P(contextNameSet[i], actionName, this->GetComputeWorker());
        this->GetComputeWorker()->GetActions().push_back(actionFinal);
        prevAction->AddChild(actionFinal);
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
    string paramName = "USER_CONSTRAINT_LIST";
    list<Apimsg_user_constraint*>* userConsList = (list<Apimsg_user_constraint*>*)this->GetComputeWorker()->GetParameter(paramName);
    list<Apimsg_user_constraint*>::iterator itU = userConsList->begin();
    list<ComputeResult*> multip2pResultList;
    list<ComputeResult*>::iterator itR;
    for (; itU != userConsList->end(); itU++)
    {
        string actionName = "Action_FinalizeServiceTopology_MP2P_";
        actionName += (*itU)->getPathId();
        string dataName = "COMPUTE_RESULT_LIST";
        list<ComputeResult*>* computeResultList = (list<ComputeResult*>*)this->GetComputeWorker()->GetContextData(this->context, actionName, dataName);
        for (itR = computeResultList->begin(); itR != computeResultList->end(); itR++)
            multip2pResultList.push_back(*itR);
    }

    list<TLV*> tlvList;
    for (itR = multip2pResultList.begin(); itR != multip2pResultList.end(); itR++)
    {
        ComputeResult* result = *itR;
        TLV* tlv = (TLV*)new char[TLV_HEAD_SIZE + sizeof(void*)];
        tlv->type = MSG_TLV_VOID_PTR;
        tlv->length = sizeof(void*);
        memcpy(tlv->value, &result, sizeof(void*));
        tlvList.push_back(tlv);
    }
    string queue = MxTCE::computeThreadPrefix + worker->GetName();
    string topic = "COMPUTE_REPLY";
    SendMessage(MSG_REPLY, queue, topic, tlvList);

    // stop out from event loop
    Action::Finish();
}




///////////////////// class Action_MP2P_ReorderPaths_MP2P  ///////////////////////////


void Action_ReorderPaths_MP2P::Process()
{
    LOG(name<<"Process() called"<<endl);
    
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
    string actionName = "Action_ComputeSchedulesWithKSP_Round2_";
    string paramName = "USER_CONSTRAINT_LIST";
    list<Apimsg_user_constraint*>* userConsList = (list<Apimsg_user_constraint*>*)this->GetComputeWorker()->GetParameter(paramName);
    list<Apimsg_user_constraint*>::iterator it = userConsList->begin();
    for (; it != userConsList->end(); it++)
    {   
        Apimsg_user_constraint* userConstraint = *it;
        actionName += userConstraint->getPathId();
        string dataName = "FEASIBLE_PATHS";
        vector<TPath*>* feasiblePaths = (vector<TPath*>*)this->GetComputeWorker()->GetContextData(this->context, actionName, dataName);
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
            char buf[256];
            snprintf(buf, 256, "Action_FinalizeServiceTopology_MP2P::Process() No feasible path found for GRI: %s, Path: %s!", 
                userConstraint->getGri().c_str(), userConstraint->getPathId().c_str());
            LOG(buf << endl);
            paramName = "TEWG";
            TEWG* tewg = (TEWG*)this->GetComputeWorker()->GetParameter(paramName);
            if (tewg)
            {
                delete tewg;
                this->GetComputeWorker()->SetParameter(paramName, NULL);
            }
            throw ComputeThreadException(buf);
        }
    }

    paramName = "TEWG";
    TEWG* tewg = (TEWG*)this->GetComputeWorker()->GetParameter(paramName);
    if (tewg)
    {
        delete tewg;
        this->GetComputeWorker()->SetParameter(paramName, NULL);
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

