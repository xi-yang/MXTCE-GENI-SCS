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

#include "resv_man.hh"
#include "resv_apiserver.hh"
#include "mxtce.hh"

// Thread specific logic
void* ResvManThread::hookRun()
{
    msgPort->SetThreadScheduler(this);

    // init RData ?

    // init push APIServer which gets pushed-in reservation data from external proxy
    ResvAPIServer revApiServer(MxTCE::resvApiServerPort, this);
    eventMaster->Schedule(&revApiServer);

    // start event loop
    // eventMaster has been initiated in parent class ThreadPortScheduler::Run() method
    eventMaster->Run();
}


// Handle message from thread message router
void ResvManThread::hookHandleMessage()
{
    Message* msg = NULL;
    while ((msg = msgPort->GetMessage()) != NULL)
    {
        msg->LogDump();
        if (msg->GetTopic() == "TEWG_RESV_REQUEST") 
        {
            Message* replyMsg = msg->Duplicate();
            replyMsg->SetType(MSG_REPLY);
            string topic = "TEWG_REPLY";
            replyMsg->SetTopic(topic);
            // use the same queue that is dedicated to computeThread
            // add reservation deltas from RData
            list<TLV*>& tlvList = replyMsg->GetTLVList();
            TEWG* tewg;
            memcpy(&tewg, (TGraph*)tlvList.front()->value, sizeof(void*));
            list<TReservation*>& resvList = RData.GetReservations();
            list<TReservation*>::iterator itr;
            for (itr = resvList.begin(); itr != resvList.end(); itr++)
                tewg->AddResvDeltas(*itr);
            this->GetMessagePort()->PostMessage(replyMsg);
        } else if (msg->GetTopic() == "TEDB_ADD_RESV_REPLY") 
        {
            TReservation* resv = (TReservation*)(msg->GetTLVList().front()->value);
            assert (resv->GetSchedules().size() > 0 && resv->GetServiceTopology() != NULL);
            RData.AddReservation(resv);
        }

        delete msg; //msg consumed
    }
}


