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

#include "scheduling.hh"
#include "log.hh"

AggregateDeltaSeries::~AggregateDeltaSeries()
{
    list<TDelta*>::iterator itd = ADS.begin();
    for (; itd != ADS.end(); itd++)
        delete (*itd);
    ADS.clear();
}

inline list<TDelta*> AggregateDeltaSeries::GetADSInWindow(time_t start, time_t end)
{
    list<TDelta*> deltaList;
    list<TDelta*>::iterator itd = ADS.begin();
    for (; itd != ADS.end(); itd++)
    {
        TDelta* delta = *itd;
        if ( (delta->GetStartTime() >= start && delta->GetStartTime() < end)
            || (delta->GetEndTime() > start && delta->GetEndTime() <= end) )
            deltaList.push_back(delta);
    }
    return deltaList;
}

inline void AggregateDeltaSeries::Insert(TDelta* delta)
{
    list<TDelta*>::iterator itd = ADS.begin();
    for (; itd != ADS.end(); itd++)
    {
        TDelta* deltaInList = *itd;
        if (deltaInList->GetStartTime() <= delta->GetEndTime())
        {
            ADS.insert(itd, delta);
            break;
        }
    }
    ADS.push_back(delta);
}


TDelta* AggregateDeltaSeries::JoinADSInWindow(time_t start, time_t end)
{
    TDelta* deltaA = NULL;
    list<TDelta*>::iterator itd = ADS.begin();
    for (; itd != ADS.end(); itd++)
    {
        TDelta* delta = *itd;
        if ( (delta->GetStartTime() >= start && delta->GetStartTime() < end)
            || (delta->GetEndTime() > start && delta->GetEndTime() <= end) )
        {
            if (deltaA == NULL) 
                deltaA = delta->Clone();
            else
                deltaA->Join(delta);
        }
    }
    return deltaA;
}

void AggregateDeltaSeries::AddDelta(TDelta* delta)
{
    delta = delta->Clone();
    if (ADS.empty())
    {
        ADS.push_back(delta);
        return;
    }

    list<TDelta*> contactDeltaList = this->GetADSInWindow(delta->GetStartTime(), delta->GetEndTime());
    if (contactDeltaList.empty())
    {
        Insert(delta);
        return;
    }

    list<TDelta*>::iterator itcd = contactDeltaList.begin();
    for (; itcd != contactDeltaList.end(); itcd++)
    {
        TDelta* deltaA1 = *itcd;
        TDelta* deltaA2 = NULL;
        if (deltaA1->GetStartTime() < delta->GetStartTime())
        {            
            // first delta in ADS list and split into deltaA1 and deltaA2
            TDelta* deltaA2 = deltaA1->Clone();
            deltaA1->SetEndTime(delta->GetStartTime());
            deltaA2->SetStartTime(delta->GetStartTime());
        }
        else if (deltaA1->GetStartTime() > delta->GetStartTime())
        {
            // split added delta
            deltaA2 = delta->Clone();
            deltaA2->SetEndTime(deltaA1->GetStartTime());
            this->Insert(deltaA2);
            delta->SetStartTime(deltaA1->GetStartTime());
            //now delta start at same time with deltaA1
            deltaA2 = deltaA1;
        }
        else // == 
        {
            deltaA2 = deltaA1;
        }
        TDelta* deltaA3 = NULL;
        if (deltaA2->GetEndTime() > delta->GetEndTime())
        {
            //current delta contains added delta and has tail (deltaA3)
            deltaA3 = deltaA2->Clone();
            deltaA2->SetEndTime(delta->GetEndTime());
            deltaA3->SetStartTime(delta->GetEndTime());
        }
        else 
        {
            delta->SetStartTime(deltaA2->GetEndTime());
        }
        deltaA2->Combine(delta);
        this->Insert(deltaA2);
        if (deltaA3)
        {
            this->Insert(deltaA3);
            delete delta;
            return;
        }
    }

    assert(delta->GetStartTime() <= delta->GetEndTime());
    if (delta->GetStartTime() == delta->GetEndTime())
        delete delta;
    else 
        this->Insert(delta);
}


// TODO: should do the same procedure above
void AggregateDeltaSeries::RemoveDelta(TDelta* delta)
{
    list<TDelta*> contactDeltaList = this->GetADSInWindow(delta->GetStartTime(), delta->GetEndTime());
    list<TDelta*>::iterator itcd = contactDeltaList.begin();
    for (; itcd != contactDeltaList.end(); itcd++)
    {
        TDelta* deltaA = *itcd;
        deltaA->Decombine(delta);
        if (((TLinkDelta*)deltaA)->GetBandwidth() == 0)
            ADS.remove(deltaA);
    }
}


AggregateDeltaSeries* AggregateDeltaSeries::Duplicate()
{
    AggregateDeltaSeries* newAds = new AggregateDeltaSeries;
    list<TDelta*>::iterator itD = this->ADS.begin();
    for (; itD != this->ADS.end(); itD++)
        newAds->GetADS().push_back((*itD)->Clone());
    return newAds;
}


void AggregateDeltaSeries::Join(AggregateDeltaSeries& ads, time_t start, time_t end)
{
    list<TDelta*> deltaList;
    if (start > 0 && end > start)
        deltaList = ads.GetADSInWindow(start, end);
    else 
        deltaList = ads.GetADS();
    list<TDelta*>::iterator itD = this->ADS.begin();
    for (; itD != this->ADS.end(); itD++)
        this->AddDelta(*itD);
}



void BandwidthAggregateGraph::AddStep(time_t t, long bw)
{
    this->TBSF[t] = bw;
}

void BandwidthAggregateGraph::LoadADS(AggregateDeltaSeries& ads, time_t start, time_t end, long capacity)
{
    // get BAG by substracting ADS bandwidth from capacity for the time window
    list<TDelta*> deltaList = ads.GetADSInWindow(start, end);
    if (deltaList.size() == 0)
    {
        AddStep(start, capacity);
        AddStep(end, 0);
        return;
    }
    list<TDelta*>::iterator itD1, itD2;
    itD1 = itD2 = deltaList.begin();
    itD2++;
    if ((*itD1)->GetStartTime() > start)
        AddStep(start, capacity);
    for (; itD1 != deltaList.end(); itD1++, itD2++)
    {
        AddStep((*itD1)->GetStartTime(), capacity - ((TLinkDelta*)(*itD1))->GetBandwidth() > 0 ? capacity -  ((TLinkDelta*)(*itD1))->GetBandwidth() : 0);
        if ((*itD1)->GetEndTime() >= end)
            break;
        if (itD2 == deltaList.end() || (itD2 != deltaList.end() && (*itD2)->GetStartTime() > (*itD1)->GetEndTime()))
        {
            AddStep((*itD1)->GetEndTime(), capacity);
        }
    }
    AddStep(end, 0);
}

void BandwidthAggregateGraph::LogDump()
{
    char buf[4096]; //up to 4K
    char str[64];
    sprintf(buf, "BAG: ");

    map<time_t, long>::iterator itA, itB;
    itA = itB = TBSF.begin();
    if (itA == TBSF.end())
    {
        strcat(buf, "[empty]");
    }
    else
        itB++;
    for (; itA != TBSF.end(); itA++, itB++)
    {
        if (itB == TBSF.end())
        {
            snprintf(str, 64, "\n\t[%ld - +INF] : bw=%ld", (*itA).first, (*itA).second);
            strcat(buf, str);
        }
        else 
        {
            snprintf(str, 64, "\n\t[%ld - %ld] : bw=%ld", (*itA).first, (*itB).first, (*itA).second);
            strcat(buf, str);
        }
    }

_output:
    strcat(buf, "\n");
    LOG_DEBUG(buf);
}


