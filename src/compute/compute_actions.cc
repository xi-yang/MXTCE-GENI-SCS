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

    string paramName = "KSP";
    vector<TPath*>* KSP = (vector<TPath*>*)this->GetComputeWorker()->GetParameter(paramName);

    // TODO: pick one or multiple paths (or return failure)
    if (KSP == NULL || KSP->size() == 0)
        throw ComputeThreadException((char*)"Action_FinalizeServiceTopology::Process() No path found!");
    
    // TODO:  translate into format API requires
    (*min_element(KSP->begin(), KSP->end(), cmp_tpath))->LogDump();
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

