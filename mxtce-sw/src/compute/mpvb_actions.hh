/*
 * Copyright (c) 2013-2014
 * GENI Project.
 * University of Maryland /Mid-Atlantic Crossroads (UMD/MAX).
 * All rights reserved.
 *
 * Created by Xi Yang 2014
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
 
#ifndef __MPVB_ACTIONS_HH__
#define __MPVB_ACTIONS_HH__

using namespace std;

#include "action.hh"
#include "tewg.hh"
#include <vector>

// TODO:  make the below configurable
#define MPVB_COMPUTE_TIMEOUT 10 // secs
#define BACKOFF_NUM 2 
#define MAX_REENTRY_NUM 10

class Action_ProcessRequestTopology_MPVB: public Action
{
    protected:
    
    public:
        Action_ProcessRequestTopology_MPVB(): Action(){ }
        Action_ProcessRequestTopology_MPVB(string& n, ComputeWorker* w): Action(n, w) { }
        virtual ~Action_ProcessRequestTopology_MPVB() { }
    
        virtual void Process();
        virtual bool ProcessChildren();
        virtual bool ProcessMessages();
        virtual void Finish();
        virtual void CleanUp();
};

class Action_PrestageCompute_MPVB: public Action
{
    protected:
    
    public:
        Action_PrestageCompute_MPVB(): Action(){ }
        Action_PrestageCompute_MPVB(string& n, ComputeWorker* w): Action(n, w) { }
        virtual ~Action_PrestageCompute_MPVB() { }
    
        virtual void Process();
        virtual bool ProcessChildren();
        virtual bool ProcessMessages();
        virtual void Finish();
        virtual void CleanUp();
};


class Action_BridgeTerminal_MPVB: public Action
{
    protected:
    
    public:
        Action_BridgeTerminal_MPVB(): Action(){ }
        Action_BridgeTerminal_MPVB(string& n, ComputeWorker* w): Action(n, w) { }
        virtual ~Action_BridgeTerminal_MPVB() { }
    
        virtual void Process();
        virtual bool ProcessChildren();
        virtual bool ProcessMessages();
        virtual void Finish();
        virtual void CleanUp();
};


class Action_FinalizeServiceTopology_MPVB: public Action
{
    protected:
    
    public:
        Action_FinalizeServiceTopology_MPVB(): Action(){ }
        Action_FinalizeServiceTopology_MPVB(string& n, ComputeWorker* w): Action(n, w) { }
        virtual ~Action_FinalizeServiceTopology_MPVB() { }
    
        virtual void Process();
        virtual bool ProcessChildren();
        virtual bool ProcessMessages();
        virtual void Finish();
        virtual void CleanUp();
};

#define MPVB_TYPE_T 1
#define MPVB_TYPE_P 2
#define MPVB_TYPE_B 3


class KSPCache {
private:
    map<TNode*, map<TNode*, vector<TPath*>*>*> cacheMap;
public:
    ~KSPCache() {
        map<TNode*, map<TNode*, vector<TPath*>*>*>::iterator itM = cacheMap.begin();
        for (; itM != cacheMap.end(); itM++) {
            map<TNode*, vector<TPath*>*>* entry = itM->second;
            map<TNode*, vector<TPath*>*>::iterator itM2 = entry->begin();
            for (; itM2 != entry->end(); itM2++) {
                // delete KSP
                vector<TPath*>* ksp = itM2->second;
                vector<TPath*>::iterator itL = ksp->begin();
                for (; itL != ksp->end(); itL++) {
                    delete (*itL);
                }
                delete ksp;
            }
            delete entry;
        }
    }

    void Add(TNode* srcNode, TNode* dstNode, vector<TPath*>* ksp) {
        if (cacheMap.find(srcNode) == cacheMap.end()) {
            cacheMap[srcNode] = new map<TNode*, vector<TPath*>*>;
        }
        map<TNode*, vector<TPath*>*>* entry = (map<TNode*, vector<TPath*>*>*)cacheMap[srcNode];
        if (entry->find(dstNode) == entry->end()) {
            (*entry)[dstNode] = ksp;
        }
    }

    vector<TPath*>* Lookup(TNode* srcNode, TNode* dstNode) {
        if (cacheMap.find(srcNode) == cacheMap.end()) 
            return NULL;
        map<TNode*, vector<TPath*>*>* entry = (map<TNode*, vector<TPath*>*>*)cacheMap[srcNode];
        if (entry->find(dstNode) == entry->end()) 
            return NULL;
        return (*entry)[dstNode];
    }            
};

#endif
