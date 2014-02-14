/*
 * Copyright (c) 2012
 * GENI Project
 * University of Maryland/Mid-Atlantic Crossroads.
 * All rights reserved.
 *
 * Created by Xi Yang 2012
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


#ifndef __WORKFLOW_HH__
#define	__WORKFLOW_HH__

#include "types.hh"
#include <vector>
#include "xmlrpc_apiserver.hh"

using namespace std;

class Resource;
class Dependency
{
protected:
    string aggregateUrn;
    string aggregateUrl;
    string hopUrn;
    Resource* resourceRef;
    vector<Dependency*> uppers; // who depend on me
    vector<Dependency*> lowers; // whom I depend on
    bool getVlanFrom;
public:
    Dependency() { Init(); }
    virtual ~Dependency() {}
    void Init() {
        aggregateUrn = "";
        aggregateUrl = "";
        hopUrn = "";
        getVlanFrom = false;
        resourceRef = NULL;
    }
    string& GetAggregateUrn() {  return aggregateUrn; }
    void SetAggregateUrn(string& urn) {  aggregateUrn = urn; }
    string& GetAggregateUrl() {  return aggregateUrl; }
    void SetAggregateUrl(string& url) {  aggregateUrl = url; }
    string& GetHopUrn() {  return hopUrn; }
    void SetHopUrn(string& urn) {  hopUrn = urn; }
    Resource* GetResourceRef() { return resourceRef; }
    void SetResourceRef(Resource* ref) { resourceRef = ref; }
    vector<Dependency*>& GetUppers() { return uppers; }
    vector<Dependency*>& GetLowers() { return lowers; }
    bool isGetVlanFrom() { return getVlanFrom; }
    bool setGetVlanFrom(bool bl) { getVlanFrom = bl; }
    bool isRoot() { return uppers.empty(); }
    bool isLeaf() { return lowers.empty(); }
    Dependency* Clone() {
        Dependency* D = new Dependency;
        D->aggregateUrn = this->aggregateUrn;
        D->aggregateUrl = this->aggregateUrl;
        D->hopUrn = this->hopUrn;
        D->resourceRef = this->resourceRef;
        //Do not clone upper and lower dependencies
        //D->getVlanFrom = this->getVlanFrom;
    }
};

class TPath;
class WorkflowData
{
protected:
    vector<Dependency*> dependencies;
    xmlrpc_c::value xmlRpcData;

protected:
    bool CheckDependencyLoop(Dependency* current, Dependency* newD);
    void LoopFreeMerge(Dependency* D1, Dependency* D2);
    xmlrpc_c::value DumpXmlRpcDataRecursive(Dependency* D);

public:
    WorkflowData() {}
    vector<Dependency*>& GetDependencies() { return dependencies; }
    virtual ~WorkflowData() {}
    virtual void LoadPath(TPath* tp);
    virtual void ComputeDependency();
    virtual void MergeDependencies(vector<Dependency*>& addDependencies);
    virtual void GenerateXmlRpcData();
    virtual xmlrpc_c::value GetXmlRpcData();
};


#endif	/* WORKFLOW_HH */

