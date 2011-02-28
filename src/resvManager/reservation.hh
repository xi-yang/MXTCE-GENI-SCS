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
     
#ifndef __RESERVATION_HH__
#define __RESERVATION_HH__

#include <list>
#include "types.hh"
#include "tewg.hh"

using namespace std;


class TSchedule
{
private:
    time_t startTime;
    time_t endTime;
    int duration; // seconds

public:
    TSchedule(time_t s, time_t e): startTime(s), endTime(e), duration(e-s) { assert (endTime > startTime); }
    TSchedule(time_t s, int d): startTime(s), endTime(s+d), duration(d) { assert (duration > 0); }
    TSchedule(time_t s, time_t e, int d): startTime(s), endTime(e), duration(d) { assert (duration > 0 && endTime - startTime >= duration); }
    virtual ~TSchedule() {}
    time_t GetStartTime() { return startTime; }
    void SetStartTime(time_t s) { startTime = s; }
    time_t GetEndTime() { return endTime; }
    void SetEndTime(time_t e) { endTime = e; }
    int GetDuration() { return duration; }
    void SetDuration(int d) { duration = d; }
    inline bool WithinSchedule(time_t t) { return (t <= endTime && t>= startTime); }
    inline bool HasScheduleIn(time_t start, time_t end) { return (startTime >= start && endTime <= end); }
    inline bool HasOverlapWith(TSchedule& sched) {
        return (this->startTime >= sched.GetStartTime() && this->startTime < sched.GetEndTime()
            || sched.GetStartTime() >= this->startTime && sched.GetStartTime() < this->endTime);
    }
    inline TSchedule* Clone() {
        TSchedule* ts = new TSchedule(startTime, endTime, duration);
        return ts;
    }
    inline bool operator==(TSchedule& sched) {
        return (this->startTime == sched.GetStartTime() && this->endTime == sched.GetEndTime() && this->duration == sched.GetDuration());
    }
};


class TDelta
{
protected:
    string resvName;
    struct timeval generatedTime;
    struct timeval appliedTime;
    TSchedule* schedule;
    Resource* targetResource;

public:
    TDelta(string& r, TSchedule* s, Resource* t): resvName(r), schedule(s), targetResource(t) {
        gettimeofday (&generatedTime, NULL);
        appliedTime.tv_sec = appliedTime.tv_usec = 0;
    }
    virtual ~TDelta() { if (schedule) delete schedule; }
    struct timeval GetGeneratedTime() { return generatedTime; }
    struct timeval GetAppliedTime() { return appliedTime; }
    TSchedule* GetSchedule() { return schedule; }
    void SetSchedule(TSchedule* s) { schedule = s; }
    Resource* GetTargetResource() { return targetResource; }
    void SetTargetResource(Resource* t) { targetResource = t; }
    virtual TDelta* Clone() = 0; 
    virtual void Apply() = 0;
    virtual void Revoke() = 0;
};

class TLinkDelta: public TDelta
{
protected:
    long bandwidth;
    
public:
    TLinkDelta(string& r, TSchedule* s, Resource* t, long bw): TDelta(r, s, t), bandwidth(bw) { }
    virtual ~TLinkDelta() { }
    long GetBandwidtdh() { return bandwidth; }
    void SetBandwidtdh(long b) { bandwidth = b; }    
    virtual TDelta* Clone(); 
    virtual void Apply(); // bandwidth only -- more activities by derived classes
    virtual void Revoke(); // bandwidth only -- more activities by derived classes
};

class TLinkDelta_PSC: public TLinkDelta
{
public:
    TLinkDelta_PSC(string& r, TSchedule* s, Resource* t, long bw): TLinkDelta(r, s, t, bw) { }
    virtual ~TLinkDelta_PSC() { }    
};

class TLinkDelta_L2SC: public TLinkDelta
{
protected:
    ConstraintTagSet vlanTags;

public:
    TLinkDelta_L2SC(string& r, TSchedule* s, Resource* t, long bw): TLinkDelta(r, s, t, bw), vlanTags(0) { }
    TLinkDelta_L2SC(string& r, TSchedule* s, Resource* t, long bw, const ConstraintTagSet& vtags): TLinkDelta(r, s, t, bw), vlanTags(vtags) { }
    virtual ~TLinkDelta_L2SC() { }    
    ConstraintTagSet& GetVlanTags() { return vlanTags; }
    void SetVlanTags(ConstraintTagSet& vtags) { vlanTags = vtags; }
};

class TLinkDelta_TDM: public TLinkDelta
{
protected:
    ConstraintTagSet timeslots;

public:
    TLinkDelta_TDM(string& r, TSchedule* s, Resource* t, long bw): TLinkDelta(r, s, t, bw), timeslots(0) { }
    TLinkDelta_TDM(string& r, TSchedule* s, Resource* t, long bw, const ConstraintTagSet& slots): TLinkDelta(r, s, t, bw), timeslots(slots) { }
    virtual ~TLinkDelta_TDM() { }    
    ConstraintTagSet& GetTimeslots() { return timeslots; }
    void SetTimeslots(ConstraintTagSet& slots) { timeslots = slots; }
};

class TLinkDelta_LSC: public TLinkDelta
{
protected:
    ConstraintTagSet wavelengths;
    
public:
    TLinkDelta_LSC(string& r, TSchedule* s, Resource* t, long bw): TLinkDelta(r, s, t, bw), wavelengths(0) { }
    TLinkDelta_LSC(string& r, TSchedule* s, Resource* t, long bw, const ConstraintTagSet& waves): TLinkDelta(r, s, t, bw), wavelengths(waves) { }
    virtual ~TLinkDelta_LSC() { }    
    ConstraintTagSet& GetWavelengths() { return wavelengths; }
    void SetWavelengths(ConstraintTagSet& waves) { wavelengths = waves; }
};


class TReservation
{
protected:
    string name;
    string status;
    TGraph* serviceTopology;
    list<TSchedule*> schedules;
    list<TDelta*> deltaCache;

public:
    TReservation(string& n): name(n) {}
    virtual ~TReservation() {}
    string& GetName() { return name; }
    void SetName(string& n) { name = n; }
    string& GeStatus() { return status; }
    void SetStatus(string& s) { status = s; }
    list<TDelta*>& GetDeltas() { return deltaCache; }
    list<TDelta*> CloneDeltas();
    void BuildDeltaCache();
    list<TDelta*> GetDeltasByResource(Resource* resource);
    list<TDelta*> GetDeltasBySchedule(TSchedule* schedule);
    void ApplyDeltas();
    void RevokeDeltas();
};


class RDatabase 
{
protected:
    list<TReservation*> reservations;

public:
    RDatabase(){ }
    virtual ~RDatabase() {}
    TReservation* LookupReservation(string& name) {
        list<TReservation*>::iterator itr = reservations.begin();
        for (; itr != reservations.end(); itr++)
            if ((*itr)->GetName() == name)
                return (*itr);
        return NULL;
    }
    void AddReservation(TReservation* resv) {
        reservations.push_back(resv);
    }
    void RemoveReservation(string& name) {
        list<TReservation*>::iterator itr = reservations.begin();
        for (; itr != reservations.end(); itr++)
            if ((*itr)->GetName() == name)
            {
                itr = reservations.erase(itr);
            }
    }
};

#endif