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

    // TODO: send back COMPUTE_REPLY message to core thread
    // TODO: if (status == _Failed)  send back failure/error message

    string queue = MxTCE::computeThreadPrefix + worker->GetName();
    string topic = "COMPUTE_REPLY";
    list<TLV*> noTLVs;
    SendMessage(MSG_REPLY, queue, topic, noTLVs);

    // destroy stored data including tewg
    string paramName = "KSP";
    vector<TPath*>* KSP = (vector<TPath*>*)this->GetComputeWorker()->GetParameter(paramName);
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
            tewg->LogDump();
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

    // TODO: should get the following params from API request
    long bw = 1000000000; // 100M
    TNode* srcNode = tewg->GetNodes().front();
    TNode* dstNode = tewg->GetNodes().back();
    //TNode* dstNode = *(++(++(++tewg->GetNodes().begin())));
    u_int32_t vtag = 4001;
    u_int32_t wave = 0;
    TSpec tspec(LINK_IFSWCAP_L2SC, LINK_IFSWCAP_ENC_ETH, bw);

    // TODO: verify ingress/egress edge Tspec

    // prune bandwidth
    tewg->PruneByBandwidth(bw);
    // compute KSP
    vector<TPath*>* KSP = new vector<TPath*>;
    try {
        tewg->ComputeKShortestPaths(srcNode, dstNode, tewg->GetNodes().size()*2, *KSP);
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
        u_int32_t vtagResult = vtag;
        u_int32_t waveResult = wave;        
        if (!(*itP)->VerifyTEConstraints(vtagResult, waveResult, tspec))
        {
            TPath* path2erase = *itP;
            itP = KSP->erase(itP);
            delete path2erase;
        }
        else
            itP++;
    }
    // store a list of ordered result paths 
    if (KSP->size() == 0)
    {
        delete KSP;
        this->GetComputeWorker()->SetParameter(paramName, NULL);
        throw ComputeThreadException((char*)"Action_ComputeKSP::Process() No KSP found after being applied with TE constraints!");
    }
    sort(KSP->begin(), KSP->end(), cmp_tpath);
    for (itP = KSP->begin(); itP != KSP->end(); itP++)
        (*itP)->LogDump();
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
        if (KSP == NULL || KSP->size() == 0)
            throw ComputeThreadException((char*)"Action_FinalizeServiceTopology::Process() No path found!");
        
        // TODO:  translate into format API requires
        (*min_element(KSP->begin(), KSP->end(), cmp_tpath))->LogDump();
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


///////////////////// class Action_CreateOrderedATS ///////////////////////////

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

// TODO: Add request parameters (max-bandwidth, max-duration, volume)
#define _BW_ 100000000 //100M
#define _Volume_ 1000000000*8 //1GB

void Action_CreateOrderedATS::Process()
{
    LOG(name<<"Process() called"<<endl);

    string paramName = "KSP";
    vector<TPath*>* KSP = (vector<TPath*>*)this->GetComputeWorker()->GetParameter(paramName);

    if (KSP == NULL || KSP->size() == 0)
        throw ComputeThreadException((char*)"Action_CreateOrderedATS::Process() Empty KSP list: no path found!");

    paramName = "TEWG";
    TEWG* tewg = (TEWG*)this->GetComputeWorker()->GetParameter(paramName);

    vector<time_t>* orderedATS = new vector<time_t>;
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

    paramName = "ORDERED_ATS";
    vector<time_t>* ats = new vector<time_t>;
    this->GetComputeWorker()->SetParameter(paramName, ats);

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
        long maxRemainBW = L->GetMaxReservableBandwidth() - ((TLinkDelta*)ads->GetADS().front())->GetBandwidth();

        // TODO: change selection and order of ATS based on request constraints and objectives!
        // the current logic prefers max-bandwidth criterion

        for (itD = ads->GetADS().begin(); itD != ads->GetADS().end(); itD++)
        {
            TDelta* delta = *itD;
            t_end = delta->GetStartTime();
            if (t_next == 0 || t_next <= t_start)
                t_next = t_end;

            //$$ judge whether link resource between (t_start, t_end) can satisfy request (before current delta)
            if (maxRemainBW*(t_end - t_start) > _Volume_)
            {
                AddUniqueTimePoint(ats, t_start);
                t_start = t_next;
            }

            if (maxRemainBW > L->GetMaxReservableBandwidth() - (((TLinkDelta*)delta)->GetBandwidth()))
                maxRemainBW = L->GetMaxReservableBandwidth() - (((TLinkDelta*)delta)->GetBandwidth());
             t_end = delta->GetEndTime();
             if (t_next <= t_start)
                t_next = t_end;

            //$$ judge whether link resource between (t_start, t_end) can satisfy request (including current delta)
            if (maxRemainBW < _BW_)
            {
                t_start = t_end = delta->GetEndTime();
                maxRemainBW = L->GetMaxReservableBandwidth();
                continue;
            }
            if (maxRemainBW*(t_end - t_start) > _Volume_)
            {
                AddUniqueTimePoint(ats, t_start);
                t_start = t_next;
            }
        }
        if (ats->size() >= MAX_ATS_SIZE)
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

void Action_ComputeSchedulesWithKSP::Process()
{
    LOG(name<<"Process() called"<<endl);
    string paramName = "TEWG";
    TEWG* tewg = (TEWG*)this->GetComputeWorker()->GetParameter(paramName);
    if (tewg == NULL)
        throw ComputeThreadException((char*)"Action_ComputeSchedulesWithKSP::Process() No TEWG available for computation!");
    
    paramName = "ORDERED_ATS";
    vector<time_t>* orderedATS = (vector<time_t>*)this->GetComputeWorker()->GetParameter(paramName);
    if (tewg == NULL)
        throw ComputeThreadException((char*)"Action_ComputeSchedulesWithKSP::Process() No Ordered Aggregate Time Series available for computation!");

    // TODO: should get the following params from API request
    long bw = 1000000000; // 100M
    TNode* srcNode = tewg->GetNodes().front();
    TNode* dstNode = tewg->GetNodes().back();
    u_int32_t vtag = 4001;
    u_int32_t wave = 0;
    time_t duration = 3600;
    TSpec tspec(LINK_IFSWCAP_L2SC, LINK_IFSWCAP_ENC_ETH, bw);
    
    // TODO: verify ingress/egress edge Tspec

    paramName = "FEASIBLE_PATHS";
    vector<TPath*>* feasiblePaths = new vector<TPath*>;
    this->GetComputeWorker()->SetParameter(paramName, feasiblePaths);

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
        tewg->PruneByBandwidth(bw);
        // $$ search KSP
        KSP.clear();
        try {
             tewg->ComputeKShortestPaths(srcNode, dstNode, tewg->GetNodes().size(), KSP);
        } catch (TCEException e) {
            //some debug logging here but do not throw exception upward
            LOG_DEBUG("Action_ComputeSchedulesWithKSP::Process() at TimePoint=" << startTime << " raised exception: " << e.GetMessage() <<endl);
        }
        // $$ verify candidate paths
        vector<TPath*>::iterator itP = KSP.begin(); 
        while (itP != KSP.end())
        {
            u_int32_t vtagResult = vtag;
            u_int32_t waveResult = wave;        
            if (!(*itP)->VerifyTEConstraints(vtagResult, waveResult, tspec))
            {
                TPath* path2erase = *itP;
                itP = KSP.erase(itP);
                delete path2erase;
            }
            else
            {
                // $$ collect results into feasible paths
                TSchedule* schedule = new TSchedule(startTime, endTime);
                vector<TPath*>::iterator itFP = feasiblePaths->begin();
                for (; itFP != feasiblePaths->end(); itFP++)
                {
                    if ((*(*itFP)) == (*(*itP)))
                    {
                        (*itFP)->GetSchedules().push_back(schedule);
                        itP++;
                        continue;
                    }
                }
                (*itP)->GetSchedules().push_back(schedule);
                feasiblePaths->push_back(*itP);
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
