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
 
#ifndef __COMPUTE_ACTIONS_HH__
#define __COMPUTE_ACTIONS_HH__

#include "action.hh"
#include <vector>


class Action_ProcessRequestTopology: public Action
{
    protected:
    
    public:
        Action_ProcessRequestTopology(): Action(){ }
        Action_ProcessRequestTopology(string& n, ComputeWorker* w): Action(n, w) { }
        virtual ~Action_ProcessRequestTopology() { }
    
        virtual void Process();
        virtual bool ProcessChildren();
        virtual bool ProcessMessages();
        virtual void Finish();
        virtual void CleanUp();
};


class Action_CreateTEWG: public Action
{
protected:

public:
    Action_CreateTEWG(): Action(){ }
    Action_CreateTEWG(string& n, ComputeWorker* w): Action(n, w) { }
    virtual ~Action_CreateTEWG() { }

    //virtual void Run();
    //virtual void Wait();
    virtual void Process();
    virtual bool ProcessChildren();
    virtual bool ProcessMessages();
    virtual void Finish();
    virtual void CleanUp();
};


class Action_ComputeKSP: public Action
{
    protected:
    
    public:
        Action_ComputeKSP(): Action(){ }
        Action_ComputeKSP(string& n, ComputeWorker* w): Action(n, w) { }
        virtual ~Action_ComputeKSP() { }
    
        virtual void Process();
        virtual bool ProcessChildren();
        virtual bool ProcessMessages();
        virtual void Finish();
        virtual void CleanUp();
};


///////////// scheduling workflow actions //////////
// ATS: Aggregate Time Series
// ADS: Aggregate Delta Series 
class Action_CreateOrderedATS: public Action
{
protected:
    u_int64_t _bandwidth; //bps
    u_int64_t _volume; // sec*bps
    inline void AddUniqueTimePoint(vector<time_t>* ats, time_t t);
    void _Init() {
        _bandwidth = 0;
        _volume = 0;
    }
    
public:
    Action_CreateOrderedATS(): Action(){ _Init(); }
    Action_CreateOrderedATS(string& n, ComputeWorker* w): Action(n, w) { _Init(); }
    virtual ~Action_CreateOrderedATS() { }
    u_int64_t GetReqBandwidth() { return _bandwidth; } 
    void SetReqBandwidth(u_int64_t b) { _bandwidth = b; }
    u_int64_t GetReqVolume() { return _volume; } 
    void SetReqVolume(u_int64_t v) { _volume = v; }

    virtual void Process();
    virtual bool ProcessChildren();
    virtual bool ProcessMessages();
    virtual void Finish();
    virtual void CleanUp();
};


#define MAX_ATS_SIZE 100 // change / configurable
class Apimsg_user_constraint;
class Action_ComputeSchedulesWithKSP: public Action
{
protected:
    Apimsg_user_constraint* userConstraint;
    bool blComputeBAG;
    u_int64_t _bandwidth; //bps
    u_int64_t _volume; // sec*bps
    
    void _Init() {
        userConstraint = NULL;
        blComputeBAG = false;
        _bandwidth = 0;
        _volume = 0;
    }

public:
    Action_ComputeSchedulesWithKSP(): Action(){ _Init(); }
    Action_ComputeSchedulesWithKSP(string& n, ComputeWorker* w): Action(n, w) { _Init(); }
    virtual ~Action_ComputeSchedulesWithKSP() { }
    Apimsg_user_constraint*  GetUserConstraint() { return userConstraint; }
    void SetUserConstraint(Apimsg_user_constraint* u) { userConstraint = u; }
    bool yesComputeBAG() { return blComputeBAG; }
    bool setComputeBAG(bool b) { blComputeBAG = b; }
    u_int64_t GetReqBandwidth() { return _bandwidth; } 
    void SetReqBandwidth(u_int64_t b) { _bandwidth = b; }
    u_int64_t GetReqVolume() { return _volume; } 
    void SetReqVolume(u_int64_t v) { _volume = v; }

    virtual void Process();
    virtual bool ProcessChildren();
    virtual bool ProcessMessages();
    virtual void Finish();
    virtual void CleanUp();
};


class TPath;
class BandwidthAvailabilityGraph;
class Action_FinalizeServiceTopology: public Action
{
protected:

public:
    Action_FinalizeServiceTopology(): Action(){ }
    Action_FinalizeServiceTopology(string& n, ComputeWorker* w): Action(n, w) { }
    virtual ~Action_FinalizeServiceTopology() { }
    
    virtual void Process();
    virtual bool ProcessChildren();
    virtual bool ProcessMessages();
    virtual void Finish();
    virtual void CleanUp();
};


#endif
