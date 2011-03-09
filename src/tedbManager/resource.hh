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

#ifndef __RESOURCE_HH__
#define __RESOURCE_HH__

#include "types.hh"
#include "utils.hh"
#include <list>
#include <map>

using namespace std;


enum ResourceType
{
    RTYPE_DOMAIN = 0x01,
    RTYPE_NODE,
    RTYPE_PORT,
    RTYPE_LINK,
    RTYPE_POINT,
};


class WorkData
{
public:
    WorkData() {}
    virtual ~WorkData() {}
};

class ResourceMapByString;
class TDelta;
class Resource
{
protected:
    ResourceType type;
    u_int32_t _id;  // unique resource ID 
    string name;    // topology identification name
    string address; // IPv address (with /slash netmask if applicable)
    list<TDelta*> deltaList;
    WorkData* workData;

public:
    Resource(ResourceType t, u_int32_t i, string& n, string& a): type(t), _id(i), name(n), address(a), workData(NULL) { }
    Resource(ResourceType t, u_int32_t i, string& n): type(t), _id(i), name(n), workData(NULL)  { address = ""; }
    Resource(ResourceType t, u_int32_t i): type(t), _id(i), workData(NULL)  { name= ""; address = ""; }
    virtual ~Resource() { if (workData != NULL) delete workData; }
    ResourceType GetType() {return type;}
    void SetType(ResourceType t) { type = t;}
    u_int32_t GetId() { return _id; }
    void SetId(u_int32_t i) { _id = i; }
    string& GetName() { return name; }
    void SetName(string& n) { name = n; }
    string& GetAddress() { return address; }
    void SetAddress(string& a) { address = a; }
    list<TDelta*>& GetDeltaList() { return deltaList; }
    void AddDelta(TDelta* delta);
    void RemoveDelta(TDelta* delta);
    void RemoveDeltasByName(string resvName);
    list<TDelta*> LookupDeltasByName(string resvName);
    WorkData* GetWorkData() { return workData; }
    void SetWorkData(WorkData* w) { workData = w; }
};


struct strcmpless {
    bool operator() (const string& ls, const string& rs) const 
    {   return (ls.compare(rs) < 0); }
};


class ResourceMapByString: public multimap<string, Resource*, strcmpless>
{
public:
    const list<Resource*> operator[](string& name) {
        multimap<string, Resource*, strcmpless>::iterator it, itlow, itup;
        itlow = this->lower_bound(name);
        itup = this->upper_bound(name);
        list<Resource*> rList;
        for ( it = itlow; it != itup; it++)
            rList.push_back((*it).second);
        return rList;
    }
};


class Node;
class Port;
class Link;
class Point;
class ISCD;
class IACD;
class NodeIfAdaptMatrix;

class Domain: public Resource
{
protected:
    map<string, Node*, strcmpless> nodes;

public:
    Domain(u_int32_t id, string& name): Resource(RTYPE_DOMAIN, id, name) { }
    Domain(u_int32_t id, string& name, string& address): Resource(RTYPE_DOMAIN, id, name, address) { }
    virtual ~Domain() { }    
    map<string, Node*, strcmpless>& GetNodes() { return nodes; }
    void AddNode(Node* node);
    bool operator==(Domain& aDomain) {
        return (this->name == aDomain.name);
    }
};

class Node: public Resource
{
protected:
    map<string, Port*, strcmpless> ports;
    Domain* domain;
    NodeIfAdaptMatrix* ifAdaptMatrix;

public:
    Node(u_int32_t id, string& name): Resource(RTYPE_NODE, id, name), domain(NULL), ifAdaptMatrix(NULL) { }
    Node(u_int32_t id, string& name, string& address): Resource(RTYPE_NODE, id, name, address), domain(NULL), ifAdaptMatrix(NULL) { }
    virtual ~Node();
    Domain* GetDomain() { return domain; }
    void SetDomain(Domain* d) { domain = d; }
    map<string, Port*, strcmpless>& GetPorts() { return ports; }
    void AddPort(Port* port);
    NodeIfAdaptMatrix* GetIfAdaptMatrix() { return ifAdaptMatrix; }
    void SetIfAdaptMatrix(NodeIfAdaptMatrix* matrix) { ifAdaptMatrix = matrix; }
    bool operator==(Node& aNode) {
        if (this->domain == NULL && aNode.domain != NULL 
            || this->domain != NULL && aNode.domain == NULL)
            return false;
        if (this->domain != NULL && aNode.domain != NULL && !(*this->domain == *aNode.domain))
            return false;
        return (this->name == aNode.name);
    }
};


class Port: public Resource
{
protected:
    map<string, Link*, strcmpless> links;
    map<string, Point*, strcmpless> points; //$$$ place holder
    Node* node;
    long maxBandwidth;
    long maxReservableBandwidth;
    long minReservableBandwidth;
    long unreservedBandwidth[8];   // 8 priorities: use unreservedBandwidth[7] by default
    long bandwidthGranularity;
    void _Init() {
        node = NULL;
        maxBandwidth = maxReservableBandwidth = minReservableBandwidth = 0;
        for (int i = 0; i < 8; i++) unreservedBandwidth[i] = 0;
        bandwidthGranularity = 0;
    }

public:
    Port(u_int32_t id, string& name): Resource(RTYPE_PORT, id, name) { _Init(); }
    Port(u_int32_t id, string& name, string& address): Resource(RTYPE_PORT, id, name, address) { _Init(); }
    virtual ~Port() { }    
    Node* GetNode() { return node; }
    void SetNode(Node* n) { node = n; }
    map<string, Link*, strcmpless>& GetLinks() { return links; }
    void AddLink(Link* link);
    map<string, Point*, strcmpless>& GetPoints() { return points; }
    void AddPoint();
    long GetMaxBandwidth() {return maxBandwidth;}
    void SetMaxBandwidth(long bw) { maxBandwidth = bw;}
    long GetMaxReservableBandwidth() {return maxReservableBandwidth;}
    void SetMaxReservableBandwidth(long bw) { maxReservableBandwidth = bw;}
    long GetMinReservableBandwidth() {return minReservableBandwidth;}
    void SetMinReservableBandwidth(long bw) { minReservableBandwidth = bw;}
    long GetBandwidthGranularity() {return bandwidthGranularity;}
    void SetBandwidthGranularity(long bw) { bandwidthGranularity = bw;}
    long* GetUnreservedBandwidth() { return unreservedBandwidth; }
    bool operator==(Port& aPort) {
        if (this->node == NULL && aPort.node != NULL 
            || this->node != NULL && aPort.node == NULL)
            return false;
        if (this->node != NULL && aPort.node != NULL && !(*this->node == *aPort.node))
            return false;
        return (this->name == aPort.name);
    }
};

#define _INF_ 2147483647

class Link: public Resource
{
protected:
    Port* port;
    int metric;
    long maxBandwidth;
    long maxReservableBandwidth;
    long minReservableBandwidth;
    long unreservedBandwidth[8];   // 8 priorities: use unreservedBandwidth[7] by default
    long bandwidthGranularity;
    Link* remoteLink;
    list<ISCD*> swCapDescriptors;
    list<IACD*> swAdaptDescriptors;
    list<Link*> containerLinks;      // the lower-layer links that this (virtual) link depends on (optional)
    list<Link*> componentLinks;     // the upper-layer (virtual) links that depends on this link (optional)
    void _Init() {
        port = NULL;
        metric = _INF_;
        maxBandwidth = maxReservableBandwidth = minReservableBandwidth = 0;
        for (int i = 0; i < 8; i++) unreservedBandwidth[i] = 0;
        bandwidthGranularity = 0;
        remoteLink = NULL;
    }

public:
    Link(u_int32_t id, string& name): Resource(RTYPE_LINK, id, name) { _Init(); }
    Link(u_int32_t id, string& name, string& address): Resource(RTYPE_LINK, id, name, address) { _Init(); }
    virtual ~Link() { }
    Port* GetPort() { return port; }
    void SetPort(Port* p) { port = p; }
    int GetMetric() {return metric;}
    void SetMetric(int x) { metric = x;}
    long GetMaxBandwidth() {return maxBandwidth;}
    void SetMaxBandwidth(long bw) { maxBandwidth = bw;}
    long GetMaxReservableBandwidth() {return maxReservableBandwidth;}
    void SetMaxReservableBandwidth(long bw) { maxReservableBandwidth = bw;}
    long GetMinReservableBandwidth() {return minReservableBandwidth;}
    void SetMinReservableBandwidth(long bw) { minReservableBandwidth = bw;}
    long GetBandwidthGranularity() {return bandwidthGranularity;}
    void SetBandwidthGranularity(long bw) { bandwidthGranularity = bw;}
    long* GetUnreservedBandwidth() { return unreservedBandwidth; }
    long GetAvailableBandwidth() { return unreservedBandwidth[7]; }
    void SetAvailableBandwidth(long bw) { unreservedBandwidth[7] = bw; }
    Link* GetRemoteLink() {return remoteLink;}
    void SetRemoteLink(Link* rmt) { remoteLink = rmt;}
    list<ISCD*>& GetSwCapDescriptors() { return swCapDescriptors; }
    list<IACD*>& GetSwAdaptDescriptors() { return swAdaptDescriptors; }
    list<Link*>& GetContainerLinks() { return containerLinks; }
    list<Link*>& GetComponentLinks() { return componentLinks; }
    bool operator==(Link& aLink) {
        if (this->port == NULL && aLink.port != NULL 
            || this->port != NULL && aLink.port == NULL)
            return false;
        if (this->port != NULL && aLink.port != NULL && !(*this->port == *aLink.port))
            return false;
        return (this->name == aLink.name);
    }
};


//$$$$ place holder for measurement, monitoring and compute resource attachment points
class Point: public Resource
{
protected:
    Port* port;

public:
    Point(u_int32_t id, string& name): Resource(RTYPE_POINT, id, name), port(NULL) { }
    Point(u_int32_t id, string& name, string& address): Resource(RTYPE_POINT, id, name, address), port(NULL) { }
    ~Point() { }
    Port* GetPort() { return port; }
    void SetPort(Port* p) { port = p; }
    bool operator==(Point& aPoint) {
        if (this->port == NULL && aPoint.port != NULL 
            || this->port != NULL && aPoint.port == NULL)
            return false;
        if (this->port != NULL && aPoint.port != NULL && !(*this->port == *aPoint.port))
            return false;
        return (this->name == aPoint.name);
    }
};


// Interface Switching Capability Descriptor (ISCD)  base classes

#define LINK_IFSWCAP_PSC1		1
#define LINK_IFSWCAP_PSC2		2
#define LINK_IFSWCAP_PSC3		3 
#define LINK_IFSWCAP_PSC4		4
#define LINK_IFSWCAP_L2SC		51
#define LINK_IFSWCAP_TDM		100
#define LINK_IFSWCAP_LSC		150
#define LINK_IFSWCAP_FSC		200

#define LINK_IFSWCAP_ENC_PKT		1
#define LINK_IFSWCAP_ENC_ETH	    2
#define LINK_IFSWCAP_ENC_PDH		3
#define LINK_IFSWCAP_ENC_RESV1		4
#define LINK_IFSWCAP_ENC_SONETSDH	5
#define LINK_IFSWCAP_ENC_RESV2		6
#define LINK_IFSWCAP_ENC_DIGIWRAP	7
#define LINK_IFSWCAP_ENC_LAMBDA		8
#define LINK_IFSWCAP_ENC_FIBER		9
#define LINK_IFSWCAP_ENC_RESV3		10
#define LINK_IFSWCAP_ENC_FIBRCHNL	11
#define LINK_IFSWCAP_ENC_G709ODUK	12
#define LINK_IFSWCAP_ENC_G709OCH	13


class ISCD
{
public:
    u_char	switchingType;
    u_char	encodingType;
    long capacity;
    ISCD (u_char swType, u_char enc, long bw): switchingType(swType), encodingType(enc), capacity(bw)  { }
    virtual ~ISCD() { }
    virtual ISCD* Duplicate() { }
};


class ISCD_PSC: public ISCD
{
public:
    int mtu;
    ISCD_PSC(int level,long bw,  int m): ISCD(LINK_IFSWCAP_PSC1+level-1, LINK_IFSWCAP_ENC_PKT, bw), mtu(m) { }
    virtual ~ISCD_PSC() { }
    virtual ISCD* Duplicate(){
        ISCD_PSC* iscd = new ISCD_PSC(encodingType-LINK_IFSWCAP_PSC1+1, this->capacity, this->mtu);
        return iscd;
    }
        
};


#ifndef MAX_VLAN_NUM
#define MAX_VLAN_NUM 4096
#define VTAG_UNTAGGED 4096
#endif

class ISCD_L2SC: public ISCD
{
public:
    int mtu;
    ConstraintTagSet availableVlanTags;
    ConstraintTagSet assignedVlanTags;
    bool vlanTranslation;

    ISCD_L2SC(long bw, int m): ISCD(LINK_IFSWCAP_L2SC, LINK_IFSWCAP_ENC_ETH, bw), mtu(m), availableVlanTags(MAX_VLAN_NUM), assignedVlanTags(MAX_VLAN_NUM), vlanTranslation(false) { }
    virtual ~ISCD_L2SC() { }
    virtual ISCD* Duplicate(){
        ISCD_L2SC* iscd = new ISCD_L2SC(this->capacity, this->mtu);
        iscd->availableVlanTags = this->availableVlanTags;
        iscd->assignedVlanTags = this->assignedVlanTags;
        iscd->vlanTranslation = this->vlanTranslation;
        return iscd;
    }
            
};


#ifndef MAX_TIMESLOTS_NUM
#define MAX_TIMESLOTS_NUM 192
#endif

class ISCD_TDM: public ISCD
{
public:
    long minReservableBandwidth;
    ConstraintTagSet availableTimeSlots;
    ConstraintTagSet assignedTimeSlots;

    ISCD_TDM(long bw, long min): ISCD(LINK_IFSWCAP_TDM, LINK_IFSWCAP_ENC_SONETSDH, bw), minReservableBandwidth(min), availableTimeSlots(MAX_TIMESLOTS_NUM), assignedTimeSlots(MAX_TIMESLOTS_NUM) { }
    ~ISCD_TDM() { }
    virtual ISCD* Duplicate(){
        ISCD_TDM* iscd = new ISCD_TDM(this->capacity, this->minReservableBandwidth);
        iscd->availableTimeSlots = this->availableTimeSlots;
        iscd->assignedTimeSlots = this->assignedTimeSlots;
        return iscd;
    }

};


#ifndef MAX_WAVE_NUM
#define MAX_WAVE_NUM 256 //64: 10G with OPVCX; 40: WDM 
#endif

class ISCD_LSC: public ISCD
{
public:
    ConstraintTagSet availableWavelengths;
    ConstraintTagSet assignedWavelengths;
    bool wavelengthTranslation;

    ISCD_LSC(long bw): ISCD(LINK_IFSWCAP_LSC, LINK_IFSWCAP_ENC_LAMBDA, bw), availableWavelengths(MAX_WAVE_NUM), assignedWavelengths(MAX_WAVE_NUM), wavelengthTranslation(false) { }
    ~ISCD_LSC() { }
    virtual ISCD* Duplicate(){
        ISCD_LSC* iscd = new ISCD_LSC(this->capacity);
        iscd->availableWavelengths = this->availableWavelengths;
        iscd->assignedWavelengths = this->assignedWavelengths;
        iscd->wavelengthTranslation = this->wavelengthTranslation;
        return iscd;
    }
};



// Interface Switching Adaptoation Descriptor

class IACD
{
public:
    u_char	lowerLayerSwitchingType;
    u_char	lowerLayerEncodingType;
    u_char  upperLayerSwitchingType;
    u_char	upperLayerEncodingType;
    long maxAdaptBandwidth;
};


// Node-Interface Switching Maxtrix

#define MATRIX_SIZE 1024  //1024 x 1024 matrix

struct portcmpless {
    bool operator() (Port* lp, Port* rp) const 
    {   return (lp->GetName().compare(rp->GetName()) < 0); }
};

class NodeIfAdaptMatrix
{
private:
    long* bwCaps;
    int size;
    int portSN;
    map<Port*, int, portcmpless> portMap;

    NodeIfAdaptMatrix() { }

public:
    NodeIfAdaptMatrix(int n): size(n), portSN(0) { bwCaps = new long[n*n]; }
    ~NodeIfAdaptMatrix() { delete[] bwCaps; }
    void AddPort(Port* port) { assert(port && portSN < size); portMap[port] = portSN++; }
    long GetAdaptCap(int n1, int n2) { 
        assert(n1 >= 0 && n1 < size && n2 >= 0 && n2 < size); 
        return bwCaps[n1*size+n2]; 
    }
    long GetAdaptCap(Port* p1, Port* p2) { 
        if (portMap.find(p1) != portMap.end() && portMap.find(p2) != portMap.end()) 
        {
            int n1 = portMap[p1];
            int n2 = portMap[p2];
            return GetAdaptCap(n1, n2); 
        }
    }
    list<Port*> GetAdaptToPorts(Port* port, long bw=0);
    list<Port*> GetAdaptFromPorts(Port* port, long bw=0);
};


#endif
