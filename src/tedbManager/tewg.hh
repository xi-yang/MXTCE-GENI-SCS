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
     
#ifndef __TEWG_HH__
#define __TEWG_HH__

#include "resource.hh"
#include <vector>

using namespace std;


class TLink;

class TSpec
{
public:    
    u_int8_t SWtype;    
    u_int8_t ENCtype;    
    long Bandwidth;    
    TSpec ():SWtype(0), ENCtype(0), Bandwidth(0) {}    
    TSpec (u_int8_t sw, u_int8_t enc, long bw):SWtype(sw), ENCtype(enc), Bandwidth(bw) {}    
    void Update(u_int8_t sw_type, u_int8_t encoding, long bw) {            
        SWtype = sw_type;            
        ENCtype = encoding;            
        Bandwidth = bw;        
    }    
    bool operator == (TSpec &t) {
        return (this->SWtype == t.SWtype && this->ENCtype == t.ENCtype && this->Bandwidth == t.Bandwidth);
    }
    bool operator <= (TSpec &t) {
        return (this->SWtype == t.SWtype && this->ENCtype == t.ENCtype && this->Bandwidth <= t.Bandwidth);           
    }
};

class TServiceSpec: public TSpec
{
public:
    ConstraintTagSet vlanSet;
    ConstraintTagSet timeslotSet;
    ConstraintTagSet wavelengthSet;
    TServiceSpec(): TSpec(), vlanSet(MAX_VLAN_NUM), timeslotSet(MAX_TIMESLOTS_NUM), wavelengthSet(MAX_WAVE_NUM) { }
    TServiceSpec(u_int8_t sw, u_int8_t enc, long bw, string& tagRange): TSpec(sw, enc, bw),
        vlanSet(MAX_VLAN_NUM), timeslotSet(MAX_TIMESLOTS_NUM), wavelengthSet(MAX_WAVE_NUM) {
        switch (sw) {
            case LINK_IFSWCAP_L2SC:
                vlanSet.LoadRangeString(tagRange);
                break;
            case LINK_IFSWCAP_TDM:
                timeslotSet.LoadRangeString(tagRange);
                break;
            case LINK_IFSWCAP_LSC:
                wavelengthSet.LoadRangeString(tagRange);
                break;
        }
    }
    ConstraintTagSet& GetVlanSet() { return vlanSet; }
    ConstraintTagSet& GetTImeslotSet() { return timeslotSet; }
    ConstraintTagSet& GetWavelengthSet() { return wavelengthSet; }
    ConstraintTagSet& GetTagSet() {
        switch (SWtype) {
            case LINK_IFSWCAP_L2SC:
                return vlanSet;
                break;
            case LINK_IFSWCAP_TDM:
                return timeslotSet;
                break;
            case LINK_IFSWCAP_LSC:
                return wavelengthSet;
                break;
        }
        return vlanSet;
    }
};


// TODO: use map<string, void*> for additional storage
class TWorkData: public WorkData
{
public:
    double linkCost;
    double pathCost;
    bool visited;
    bool filteroff;
    TSpec tspec;
    list<TLink*> path;

public:
    TWorkData(): WorkData(), linkCost(_INF_), pathCost(_INF_), visited(false), filteroff(false) { }
    TWorkData(double lc, double pc): WorkData(), linkCost(lc), pathCost(pc), visited(false), filteroff(false) { }
    ~TWorkData() {}
    void Cleanup() {
        linkCost = _INF_;
        pathCost = _INF_;
        visited = false;
        filteroff = false;
        tspec.Update(0, 0, 0);
        path.clear();
    }
};

#define TWDATA(x) ((TWorkData*)((x)->GetWorkData()))

// TGraph component classes

// Rule of Thumb: resources shoud be constructed in domain->node->port->link order and destroyed in reverse order

class TDomain: public Domain
{
protected:
    bool disabled;
    
public:
    TDomain(u_int32_t id, string& name): Domain(id, name), disabled(false) { }
    TDomain(u_int32_t id, string& name, string& address): Domain(id, name, address), disabled(false) { }
    virtual ~TDomain() { }
    bool IsDisabled() { return disabled; }
    void SetDisabled(bool d) { disabled = d; }
    TDomain* Clone(bool newSubLevels=true);
    bool operator==(TDomain& aDomain) {
        return (this->name == aDomain.name);
    }
};


class TNode: public Node
{
protected:
    bool disabled;
    bool visited;
    list<TLink*> lclLinks;
    list<TLink*> rmtLinks;

public:
    TNode(u_int32_t id, string& name): Node(id, name), disabled(false), visited(false) { }
    TNode(u_int32_t id, string& name, string& address): Node(id, name, address), disabled(false) { }
    virtual ~TNode() { }
    bool IsDisabled() { return disabled; }
    void SetDisabled(bool d) { disabled = d; }
    list<TLink*>& GetLocalLinks() { return lclLinks; }
    void AddLocalLink(TLink* link);
    bool HasLocalLink(TLink* link);
    list<TLink*>& GetRemoteLinks() { return rmtLinks; }
    void AddRemoteLink(TLink* link);
    bool HasRemoteLink(TLink* link);
    TNode* Clone(bool newSubLevels=true);
    bool operator==(TNode& aNode) {
        if (this->domain == NULL && aNode.domain != NULL 
            || this->domain != NULL && aNode.domain == NULL)
            return false;
        if (this->domain != NULL && aNode.domain != NULL && !(*this->domain == *aNode.domain))
            return false;
        return (this->name == aNode.name);
    }
    bool operator<(TNode& aNode) {
        assert (this->workData != NULL && aNode.GetWorkData() != NULL);
        return (((TWorkData*)this->workData)->pathCost < ((TWorkData*)aNode.GetWorkData())->pathCost);
    }
};


class TPort: public Port
{
protected:
    bool disabled;
    bool visited;

public:
    TPort(u_int32_t id, string& name): Port(id, name), disabled(false), visited(false) { }
    TPort(u_int32_t id, string& name, string& address): Port(id, name, address), disabled(false) { }
    virtual ~TPort() { }
    bool IsDisabled() { return disabled; }
    void SetDisabled(bool d) { disabled = d; }
    bool HasLink();
    TPort* Clone(bool newSubLevels=true);
    bool operator==(TPort& aPort) {
        if (this->node == NULL && aPort.node != NULL 
            || this->node != NULL && aPort.node == NULL)
            return false;
        if (this->node != NULL && aPort.node != NULL && !(*this->node == *aPort.node))
            return false;
        return (this->name == aPort.name);
    }
};


class TLink: public Link
{
protected:
    bool disabled;
    bool visited;
    bool edgeOnly;
    TNode* lclEnd;
    TNode* rmtEnd;

public:
    TLink(u_int32_t id, string& name): Link(id, name),disabled(false), visited(false), edgeOnly(false), lclEnd(NULL), rmtEnd(NULL) { }
    TLink(u_int32_t id, string& name, string& address): Link(id, name, address), disabled(false), edgeOnly(false), lclEnd(NULL), rmtEnd(NULL) { }
    virtual ~TLink() { }
    bool IsDisabled() { return disabled; }
    void SetDisabled(bool d) { disabled = d; }
    bool IsEdge() { return edgeOnly; }
    void SetEdge(bool e) { edgeOnly = e; }
    TNode* GetLocalEnd() { return lclEnd; }
    void SetLocalEnd(TNode* lcl) { lclEnd = lcl; }
    TNode* GetRemoteEnd() { return rmtEnd; }
    void SetRemoteEnd(TNode* rmt) { rmtEnd = rmt; }    
    bool VerifyEdgeLink();
    bool VerifyRemoteLink();
    bool VerifyFullLink();
    ISCD* GetTheISCD();
    TLink* Clone();
    // path computation helpers
    bool IsAvailableForTspec(TSpec& tspec);
    bool VerifyEdgeLinkTSpec(TSpec& tspec);
    bool CanBeLastHopTrunk(TSpec& tspec);
    void ExcludeAllocatedVtags(ConstraintTagSet &vtagset);
    void ProceedByUpdatingVtags(ConstraintTagSet &head_vtagset, ConstraintTagSet &next_vtagset, bool do_translation=false);
    void ProceedByUpdatingWaves(ConstraintTagSet &head_waveset, ConstraintTagSet &next_waveset);
    void ProceedByUpdatingTimeslots(ConstraintTagSet &head_timeslotset, ConstraintTagSet &next_timeslotset);
    bool CrossingRegionBoundary(TSpec& tspec, TLink* next_link=NULL);
    bool GetNextRegionTspec(TSpec& tspec, TLink* next_link=NULL);
    // operators
    bool operator==(TLink& aLink) {
        if (this->port == NULL && aLink.port != NULL 
            || this->port != NULL && aLink.port == NULL)
            return false;
        if (this->port != NULL && aLink.port != NULL && !(*this->port == *aLink.port))
            return false;
        return (this->name == aLink.name);
    }
    bool operator<(TLink& aLink) {
        assert (this->workData != NULL && aLink.GetWorkData() != NULL);
        return (((TWorkData*)this->workData)->pathCost < ((TWorkData*)aLink.GetWorkData())->pathCost);
    }
};


class TGraph
{
protected:
    string name;
    list<TDomain*> tDomains;
    list<TNode*> tNodes;
    list<TPort*> tPorts;
    list<TLink*> tLinks;

public:
    TGraph(string& n): name(n) { }
    virtual ~TGraph() { }
    list<TDomain*>& GetDomains() { return tDomains; }
    list<TNode*>& GetNodes() { return tNodes; }
    list<TPort*>& GetPorts() { return tPorts; }
    list<TLink*>& GetLinks() { return tLinks; }
    void AddDomain(TDomain* domain);
    void AddNode(TDomain* domain, TNode* node);
    void RemoveNode(TNode* node);
    void AddPort(TNode* node, TPort* port);
    void RemovePort(TPort* port);
    void AddLink(TPort* port, TLink* link);
    void AddLink(TNode* node, TLink* link);
    void RemoveLink(TLink* link);
    TDomain* LookupDomainByName(string& name);
    TDomain* LookupDomainByURN(string& urn);
    TNode* LookupNodeByURN(string& urn);
    TPort* LookupPortByURN(string& urn);
    TLink* LookupLinkByURN(string& urn);
    void LoadPath(list<TLink*> path);
    TGraph* Clone();
    void LogDump();
};


class TSchedule;
class BandwidthAvailabilityGraph;
class TPath {
protected:
    list<TLink*> path;
    double cost;
    list<TLink*> maskedLinkList; // links that have filteroff = true
    TNode* deviationNode;
    list<TSchedule*> schedules; // feasible schedules on the path
    bool independent;
    BandwidthAvailabilityGraph* bag;

public:
    TPath(): cost(_INF_), deviationNode(NULL), independent(false), bag(NULL) {}
    ~TPath();
    list<TLink*>& GetPath() { return path; }
    void SetPath(list<TLink*>& p) { path.assign(p.begin(), p.end()); }
    double GetCost() { return cost; }
    void SetCost(double c) { cost = c; }
    list<TLink*>& GetMaskedLinkList() { return maskedLinkList; }
    bool IsIndependent() { return independent; }
    void SetIndependent(bool b) { independent = b; }
    BandwidthAvailabilityGraph* GetBAG() { return bag; }
    void SetBAG(BandwidthAvailabilityGraph* b) { bag = b; }
    void FilterOffMaskedLinks(bool bl) { 
            list<TLink*>::iterator itLink;
            for (itLink = maskedLinkList.begin(); itLink != maskedLinkList.end(); itLink++) {
                TWDATA(*itLink)->filteroff = bl;
            }
        }
    TNode* GetDeviationNode() { return deviationNode; }
    void SetDeviationNode(TNode* node) { deviationNode = node; }
    list<TSchedule*>& GetSchedules() { return schedules; }
    void CalculatePathCost() {
            list<TLink*>::iterator itLink;
            if (path.size() == 0)
            {
                cost = _INF_;
                return;
            }                
            cost=0;
            for (itLink = path.begin(); itLink != path.end(); itLink++) {
                cost += (*itLink)->GetMetric();
            }
        }
    void ExpandWithRemoteLinks();
    bool VerifyTEConstraints(TServiceSpec& ingTSS,TServiceSpec& egrTSS);
    void UpdateLayerSpecInfo(u_int32_t srcVtag, u_int32_t dstVtag);
    BandwidthAvailabilityGraph* CreatePathBAG(time_t start, time_t end);
    void Cleanup() {
        path.clear();
        cost = _INF_;
        maskedLinkList.clear();
        deviationNode = NULL;
    }
    bool operator< (TPath& p) const { return this->cost < p.cost; }
    bool operator== (TPath& p) const { 
        if (path.size() != p.path.size())
            return false;
        list<TLink*>::const_iterator itL1 = this->path.begin();
        list<TLink*>::const_iterator itL2 = p.path.begin();
        for (; itL1 != path.end(); itL1++, itL2++)
            if ((*itL1) != (*itL2))
                return false;
        return true;
    }
    TPath* Clone(bool doExpandRemoteLink=false);
    void LogDump();
};

extern bool cmp_tpath(TPath* p1, TPath* p2);
    
class TDelta;
class TReservation;
class TEWG: public TGraph
{
protected:
    list<TDelta*> deltaList;

public:
    TEWG(string& name): TGraph(name) {}
    virtual ~TEWG() {} // TODO: free memory of resources (cloned from TEDB) and deltas's (cloned from RDatabase) 
    list<TDelta*>& GetDeltaList() { return deltaList; }
    void AddResvDeltas(TReservation* resv);
    void RemoveResvDeltas(string& resvName);
    void HoldResvDeltas(string& resvName, bool doHold);
    void ApplyResvDeltas(string& resvName);
    void RevokeResvDeltas(string& resvName);
    void PruneByBandwidth(long bw);
    list<TLink*> ComputeDijkstraPath(TNode* srcNode, TNode* dstLink, bool cleanStart=false);
    void ComputeKShortestPaths(TNode* srcNode, TNode* dstNode, int K, vector<TPath*>& KSP);
    void LogDumpWithFlags();
};



#endif
