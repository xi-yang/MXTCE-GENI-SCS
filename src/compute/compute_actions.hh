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


class TEWG;
class Action_CreateTEWG: public Action
{
protected:
    TEWG* _tewg;
    void _Init() {
        _tewg = NULL;
    }

public:
    Action_CreateTEWG(): Action(){ _Init(); }
    Action_CreateTEWG(string& n, ComputeWorker* w): Action(n, w) { _Init(); }
    Action_CreateTEWG(string& c, string& n, ComputeWorker* w): Action(c, n, w) { _Init(); }
    virtual ~Action_CreateTEWG() { }
    virtual void* GetData(string& dataName);

    virtual void Process();
    virtual bool ProcessChildren();
    virtual bool ProcessMessages();
    virtual void Finish();
    virtual void CleanUp();
};


#define MAX_KSP_K 20 // change / configurable
class TPath;
class Apimsg_user_constraint;
class Action_ComputeKSP: public Action
{
protected:
    u_int64_t _bandwidth; //bps
    u_int64_t _volume; // sec*bps
    vector<TPath*>* _feasiblePaths;
    Apimsg_user_constraint* _userConstraint;
    void _Init() {
        _bandwidth = 0;
        _volume = 0;
        _feasiblePaths = NULL;
        _userConstraint = NULL;
    }

public:
    Action_ComputeKSP(): Action(){ _Init(); }
    Action_ComputeKSP(string& n, ComputeWorker* w): Action(n, w) { _Init(); }
    Action_ComputeKSP(string& c, string& n, ComputeWorker* w): Action(c, n, w) { _Init(); }
    virtual ~Action_ComputeKSP() { }
    u_int64_t GetReqBandwidth() { return _bandwidth; } 
    void SetReqBandwidth(u_int64_t b) { _bandwidth = b; }
    u_int64_t GetReqVolume() { return _volume; } 
    void SetReqVolume(u_int64_t v) { _volume = v; }
    virtual void* GetData(string& dataName);

    virtual void Process();
    virtual bool ProcessChildren();
    virtual bool ProcessMessages();
    virtual void Finish();
    virtual void CleanUp();
};


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



///////////// scheduling workflow actions //////////
// ATS: Aggregate Time Series
// ADS: Aggregate Delta Series 

#define MAX_ATS_SIZE 100 // change / configurable

class Action_CreateOrderedATS: public Action
{
protected:
    u_int64_t _bandwidth; //bps
    u_int64_t _volume; // sec*bps
    vector<time_t>* _orderedATS;
    inline void AddUniqueTimePoint(vector<time_t>* ats, time_t t);
    void _Init() {
        _bandwidth = 0;
        _volume = 0;
        _orderedATS = NULL;
    }
    
public:
    Action_CreateOrderedATS(): Action(){ _Init(); }
    Action_CreateOrderedATS(string& n, ComputeWorker* w): Action(n, w) { _Init(); }
    Action_CreateOrderedATS(string& c, string& n, ComputeWorker* w): Action(c, n, w) { _Init(); }
    virtual ~Action_CreateOrderedATS() { }
    u_int64_t GetReqBandwidth() { return _bandwidth; } 
    void SetReqBandwidth(u_int64_t b) { _bandwidth = b; }
    u_int64_t GetReqVolume() { return _volume; } 
    void SetReqVolume(u_int64_t v) { _volume = v; }
    virtual void* GetData(string& dataName);

    virtual void Process();
    virtual bool ProcessChildren();
    virtual bool ProcessMessages();
    virtual void Finish();
    virtual void CleanUp();
};


class Action_ComputeSchedulesWithKSP: public Action
{
protected:
    u_int64_t _bandwidth; //bps
    u_int64_t _volume; // sec*bps
    vector<TPath*>* _feasiblePaths;
    Apimsg_user_constraint* _userConstraint;
    bool blComputeBAG;
    bool blCommitBestPathToTEWG; // TODO: Improve path selection crieria (using bestPath and first available schedule for now)
    void _Init() {
        _bandwidth = 0;
        _volume = 0;
        _feasiblePaths = NULL;
        _userConstraint = NULL;
        blComputeBAG = false;
        blCommitBestPathToTEWG = false;
    }

public:
    Action_ComputeSchedulesWithKSP(): Action(){ _Init(); }
    Action_ComputeSchedulesWithKSP(string& n, ComputeWorker* w): Action(n, w) { _Init(); }
    Action_ComputeSchedulesWithKSP(string& c, string& n, ComputeWorker* w): Action(c, n, w) { _Init(); }
    virtual ~Action_ComputeSchedulesWithKSP() { }
    u_int64_t GetReqBandwidth() { return _bandwidth; } 
    void SetReqBandwidth(u_int64_t b) { _bandwidth = b; }
    u_int64_t GetReqVolume() { return _volume; } 
    void SetReqVolume(u_int64_t v) { _volume = v; }
    Apimsg_user_constraint*  GetUserConstraint() { return _userConstraint; }
    void SetUserConstraint(Apimsg_user_constraint* u) { _userConstraint = u; }
    bool yesComputeBAG() { return blComputeBAG; }
    bool SetComputeBAG(bool b) { blComputeBAG = b; }
    bool yesCommitBestPathToTEWG() { return blCommitBestPathToTEWG; }
    bool SetCommitBestPathToTEWG(bool b) { blCommitBestPathToTEWG = b; }
    virtual void* GetData(string& dataName);

    virtual void Process();
    virtual bool ProcessChildren();
    virtual bool ProcessMessages();
    virtual void Finish();
    virtual void CleanUp();
};


class Action_ProcessRequestTopology_MP2P: public Action
{
protected:

public:
    Action_ProcessRequestTopology_MP2P(): Action(){ }
    Action_ProcessRequestTopology_MP2P(string& n, ComputeWorker* w): Action(n, w) { }
    Action_ProcessRequestTopology_MP2P(string& c, string& n, ComputeWorker* w): Action(c, n, w) { }
    virtual ~Action_ProcessRequestTopology_MP2P() { }

    virtual void Process();
    virtual bool ProcessChildren();
    virtual bool ProcessMessages();
    virtual void Finish();
    virtual void CleanUp();
};



#define MAX_SCHEDULE_DURATION 1314000 // 365 days
#define BANDWIDTH_TIME_FACTOR 1.0 // 0.5 ?

class Action_ReorderPaths_MP2P: public Action
{
protected:
    time_t OverlappingTime(time_t st1, time_t et1, time_t st2, time_t et2);
    time_t GetPathOverlappingTime(TPath* path1, TPath* path2);
    double BandwidthWeightedHopLength(TPath* P);
    double SumOfBandwidthTimeWeightedCommonLinks(TPath* P, vector<TPath*>& Paths);
    void Swap(Action_ComputeSchedulesWithKSP* &ksp_i, Action_ComputeSchedulesWithKSP* &ksp_j);

public:
    Action_ReorderPaths_MP2P(): Action(){ }
    Action_ReorderPaths_MP2P(string& n, ComputeWorker* w): Action(n, w) { }
    Action_ReorderPaths_MP2P(string& c, string& n, ComputeWorker* w): Action(c, n, w) { }
    virtual ~Action_ReorderPaths_MP2P() { }

    virtual void Process();
    virtual bool ProcessChildren();
    virtual bool ProcessMessages();
    virtual void Finish();
    virtual void CleanUp();
};

class ComputeResult;
class Action_FinalizeServiceTopology_MP2P: public Action
{
protected:
    list<ComputeResult*>* _computeResultList;
    void _Init(){
        _computeResultList = NULL;
    }

public:
    Action_FinalizeServiceTopology_MP2P(): Action(){ _Init(); }
    Action_FinalizeServiceTopology_MP2P(string& n, ComputeWorker* w): Action(n, w) { _Init(); }
    Action_FinalizeServiceTopology_MP2P(string& c, string& n, ComputeWorker* w): Action(c, n, w) { _Init(); }
    virtual ~Action_FinalizeServiceTopology_MP2P() { }
    virtual void* GetData(string& dataName);

    virtual void Process();
    virtual bool ProcessChildren();
    virtual bool ProcessMessages();
    virtual void Finish();
    virtual void CleanUp();
};


#endif
