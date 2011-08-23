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
 
#ifndef __SCHEDULING_HH__
#define __SCHEDULING_HH__

#include <list>
#include "types.hh"
#include "reservation.hh"

using namespace std;

class AggregateDeltaSeries
{
protected:
    list<TDelta*> ADS;

public:
    AggregateDeltaSeries() { }
    virtual ~AggregateDeltaSeries();
    list<TDelta*>& GetADS() { return ADS; }
    inline list<TDelta*> GetADSInWindow(time_t start, time_t end);
    inline void Insert(TDelta* delta);
    TDelta* JoinADSInWindow(time_t start, time_t end);
    void AddDelta(TDelta* delta, bool doJoinInsteadOfCombine=false);
    void RemoveDelta(TDelta* delta);
    AggregateDeltaSeries* Duplicate();
    void Join(AggregateDeltaSeries& ads, time_t start=0, time_t end=0);
};

class BandwidthAvailabilityGraph
{
protected:
    map<time_t, long> TBSF; //Time-Bandwidth-Step-Function (first element is for startTime, last element always has BW value 0)

public:
    BandwidthAvailabilityGraph() { }
    virtual ~BandwidthAvailabilityGraph() { }
    map<time_t, long>& GetTBSF() { return TBSF; }
    void AddStep(time_t t, long bw);
    void LoadADS(AggregateDeltaSeries& ads, time_t start, time_t end, long capacity);
    void LogDump();
    BandwidthAvailabilityGraph* Clone();
    
    //$$ methods to convert into other representation
};

#endif
