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

#include "workflow.hh"
#include "tewg.hh"
#include "rspec.hh"
#include <map>

// TODO: Exception throwing and handling

bool WorkflowData::CheckDependencyLoop(Dependency* current, Dependency* newD)
{
    // DSF return true if encountering 'current' node
    vector<Dependency*>::iterator itLower = newD->GetLowers().begin();
    for (; itLower != newD->GetLowers().end(); itLower++)
    {
        if (*itLower == current)
            return true;
        if (CheckDependencyLoop(current, *itLower))
            return true;
    }
    return false;
}

// add D2 as lower to D1's depending uppers
void WorkflowData::LoopFreeMerge(Dependency* D1, Dependency* D2)
{
    vector<Dependency*>::iterator itD1 = D1->GetUppers().begin();
    while (itD1 != D1->GetUppers().end())
    {
        Dependency* aD1 = *itD1;
        vector<Dependency*>::iterator itD2 = D2->GetUppers().begin();
        while (itD2 != D2->GetUppers().end())
        {
            Dependency* aD2 = *itD2;
            if (aD2 == aD1)
                break;
            itD2++;
        }
        // found a new depending upper aD1 for D2
        if (itD2 == D2->GetUppers().end()
            && !this->CheckDependencyLoop(aD1, D2))
        {
            // loop-free adding D2 as dependency to the new depending upper aD1
            aD1->GetLowers().push_back(D2);
            D2->GetUppers().push_back(aD1);
        }
        itD1++;
    }
}

void WorkflowData::LoadPath(TPath* tp)
{
    list<TLink*>::iterator itp = tp->GetPath().begin();
    for (; itp != tp->GetPath().end(); itp++)
    {
        TLink* L = *itp;
        Dependency* D = new Dependency();
        D->SetHopUrn(L->GetName());
        string domainId = GetUrnField(L->GetName(), "domain");
        if (GeniAdRSpec::aggregateUrnMap.find(domainId) != GeniAdRSpec::aggregateUrnMap.end())
        {
            D->SetAggregateUrn(GeniAdRSpec::aggregateUrnMap[domainId]);
        }
        if (GeniAdRSpec::aggregateUrlMap.find(domainId) != GeniAdRSpec::aggregateUrlMap.end())
        {
            D->SetAggregateUrl(GeniAdRSpec::aggregateUrlMap[domainId]);
        }
        D->SetResourceRef(L);
        D->setGetVlanFrom(false);
        this->dependencies.push_back(D);
    }
}

void WorkflowData::ComputeDependency()
{
    // 1. Create producing-consuming dependency relationships
    vector<Dependency*>::iterator itD1 = dependencies.begin();
    for (; itD1 != dependencies.end(); itD1++)
    {
        Dependency* D1 = *itD1;
        TLink* L1 = (TLink*)D1->GetResourceRef();
        vector<Dependency*>::iterator itD2 = itD1;
        itD2++;
        if (itD2 != dependencies.end())
        {
            Dependency* D2 = *itD2;
            string domain1 = GetUrnField(D1->GetHopUrn(), "domain");
            string domain2 = GetUrnField(D2->GetHopUrn(), "domain");
            // skip same domain/aggregate dependency
            if (domain1.compare(domain2) == 0)
                continue;
            TLink* L2 = (TLink*)D2->GetResourceRef();
            // D1 depends on D2
            if (L2->GetCapabilities().find("producer") != L2->GetCapabilities().end()
                && L1->GetCapabilities().find("producer") == L1->GetCapabilities().end()
                && L1->GetCapabilities().find("consumer") != L1->GetCapabilities().end())
            {
                D1->GetLowers().push_back(D2);
                D2->GetUppers().push_back(D1);
                D1->setGetVlanFrom(true);
            }
            // D2 depends on D1
            if (L1->GetCapabilities().find("producer") != L1->GetCapabilities().end()
                && L2->GetCapabilities().find("producer") == L2->GetCapabilities().end()
                && L2->GetCapabilities().find("consumer") != L2->GetCapabilities().end())
            {
                D2->GetLowers().push_back(D1);
                D1->GetUppers().push_back(D2);
                D2->setGetVlanFrom(true);
            }
        }
    }
    
    // 2. Create continuous VLAN dependency relationships
    // 3. Create VLAN translation dependency relationships
    itD1 = dependencies.begin();
    for (; itD1 != dependencies.end(); itD1++)
    {
        Dependency* D1 = *itD1;
        TLink* L1 = (TLink*)D1->GetResourceRef();
        list<ISCD*>::iterator itS1 = L1->GetSwCapDescriptors().begin();
        for (; itS1 != L1->GetSwCapDescriptors().end(); itS1++)
        {
            if ((*itS1)->switchingType == LINK_IFSWCAP_L2SC)
            {
                break;
            }
        }
        if (itS1 == L1->GetSwCapDescriptors().end())
            continue;
        ISCD_L2SC* iscd1 = (ISCD_L2SC*)(*itS1);
        int numSameDomainHops1 = 1;
        ConstraintTagSet availableVlanRange1 = iscd1->availableVlanTags;
        vector<Dependency*>::iterator itDx = dependencies.begin();
        for (; itDx != dependencies.end(); itDx++)
        {
            if (itDx == itD1)
                continue;
            Dependency* Dx = *itDx;
            string domain1 = GetUrnField(D1->GetHopUrn(), "domain");
            string domain2 = GetUrnField(Dx->GetHopUrn(), "domain");
            if (domain1.compare(domain2) != 0)
                continue;
            // find same domain hops and get the number
            numSameDomainHops1++;
            // VLAN range intersection
            TLink* Lx = (TLink*)Dx->GetResourceRef();
            list<ISCD*>::iterator itSx = Lx->GetSwCapDescriptors().begin();
            for (; itSx != L1->GetSwCapDescriptors().end(); itSx++)
            {
                if ((*itSx)->switchingType == LINK_IFSWCAP_L2SC)
                {
                    break;
                }
            }
            if (itSx == Lx->GetSwCapDescriptors().end())
                continue;
            ISCD_L2SC* iscdx = (ISCD_L2SC*)(*itSx);
            availableVlanRange1.Intersect(iscdx->availableVlanTags);
        }
        vector<Dependency*>::iterator itD2 = itD1;
        itD2++;
        if (itD2 != dependencies.end())
        {
            Dependency* D2 = *itD2;
            string domain1 = GetUrnField(D1->GetHopUrn(), "domain");
            string domain2 = GetUrnField(D2->GetHopUrn(), "domain");
            // skip same domain/aggregate dependency
            if (domain1.compare(domain2) == 0)
                continue;
            TLink* L2 = (TLink*)D2->GetResourceRef();
            list<ISCD*>::iterator itS2 = L2->GetSwCapDescriptors().begin();
            for (; itS2 != L2->GetSwCapDescriptors().end(); itS2++)
            {
                if ((*itS2)->switchingType == LINK_IFSWCAP_L2SC)
                {
                    break;
                }
            }
            if (itS2 == L2->GetSwCapDescriptors().end())
                continue;
            ISCD_L2SC* iscd2 = (ISCD_L2SC*)(*itS2);
            int numSameDomainHops2 = 1;
            ConstraintTagSet availableVlanRange2 = iscd2->availableVlanTags;
            vector<Dependency*>::iterator itDy = dependencies.begin();
            for (; itDy != dependencies.end(); itDy++)
            {
                if (itDy == itD2)
                    continue;
                Dependency* Dy = *itDy;
                string domain1 = GetUrnField(D2->GetHopUrn(), "domain");
                string domain2 = GetUrnField(Dy->GetHopUrn(), "domain");
                if (domain1.compare(domain2) != 0)
                    continue;
                // find same domain hops and get the number
                numSameDomainHops2++;
                // VLAN range intersection
                TLink* Ly= (TLink*)Dy->GetResourceRef();
                list<ISCD*>::iterator itSy = Ly->GetSwCapDescriptors().begin();
                for (; itSy != L1->GetSwCapDescriptors().end(); itSy++)
                {
                    if ((*itSy)->switchingType == LINK_IFSWCAP_L2SC)
                    {
                        break;
                    }
                }
                if (itSy == Ly->GetSwCapDescriptors().end())
                    continue;
                ISCD_L2SC* iscdy = (ISCD_L2SC*)(*itSy);
                availableVlanRange2.Intersect(iscdy->availableVlanTags);
            }
            // check loop
            bool loop_d1_d2 = this->CheckDependencyLoop(D1, D2);
            bool loop_d2_d1 = this->CheckDependencyLoop(D2, D1);
            // Create continuous VLAN dependency relationships
            if (!iscd1->vlanTranslation && !iscd2->vlanTranslation)
            {
                // D1 depends on D2 (narrower vlan range takes higher priority) 
                // also make sure no loop if adding the dependency
                if (!loop_d1_d2 
                    && availableVlanRange1.Size()+numSameDomainHops1*4000 
                       > availableVlanRange2.Size()+numSameDomainHops2*4000)
                {
                    D1->GetLowers().push_back(D2);
                    D2->GetUppers().push_back(D1);
                    D1->setGetVlanFrom(true);
                }
                // D2 depends on D1 and no loop if adding the dependency
                if (!loop_d2_d1 
                    && availableVlanRange1.Size()+numSameDomainHops1*4000 
                       < availableVlanRange2.Size()+numSameDomainHops2*4000)
                {
                    D2->GetLowers().push_back(D1);
                    D1->GetUppers().push_back(D2);
                    D2->setGetVlanFrom(true);
                }                
            }
            // Create VLAN translation dependency relationships
            else if (iscd1->vlanTranslation && !iscd2->vlanTranslation)
            {
                // D1 depends on D2 (non-translation takes higher priority)
                // also make sure no loop if adding the dependency
                if (!loop_d1_d2)
                {
                    D1->GetLowers().push_back(D2);
                    D2->GetUppers().push_back(D1);
                    D1->setGetVlanFrom(true);
                }
            }
            else if (iscd1->vlanTranslation && !iscd2->vlanTranslation)
            {
                // D2 depends on D1 and no loop if adding the dependency
                if (!loop_d2_d1)
                {
                    D2->GetLowers().push_back(D1);
                    D1->GetUppers().push_back(D2);
                    D2->setGetVlanFrom(true);
                }                
            }            
        }
    }
        
    // 4a.  we can skip all the standalone blocks
    /*
    itD1 = dependencies.begin();
    while (itD1 != dependencies.end())
    {
        Dependency* D1 = *itD1;
        if (D1->isRoot() && D1->isLeaf())
        {
            itD1 = dependencies.erase(itD1);
            continue;
        }
        itD1++;
    }
    */
    // 4b. or, give a hop the same dependency relationships as other hops in its domain
    itD1 = dependencies.begin();
    for (; itD1 != dependencies.end(); itD1++)
    {
        Dependency* D1 = *itD1;
        vector<Dependency*>::iterator itD2 = itD1;
        for (; itD2 != dependencies.end(); itD2++)
        {
            if (itD2 == itD1)
                continue;
            Dependency* D2 = *itD2;
            string domain1 = GetUrnField(D1->GetHopUrn(), "domain");
            string domain2 = GetUrnField(D2->GetHopUrn(), "domain");
            if (domain1.compare(domain2) != 0)
                continue;
            LoopFreeMerge(D1, D2);
            LoopFreeMerge(D2, D1);
        }
    }
}

// generating a 'struct' member whose value is an array of 'dependencies'
void WorkflowData::GenerateXmlRpcData()
{
    if (dependencies.empty())
        return;
    vector<xmlrpc_c::value> arrayData;
    vector<Dependency*>::iterator itD = dependencies.begin();
    for (; itD != dependencies.end(); itD++)
    {
        if ((*itD)->isRoot()) 
        {
            arrayData.push_back(DumpXmlRpcDataRecursive(*itD));
        }
    }
    xmlrpc_c::value_array anArray(arrayData);
    map<string, xmlrpc_c::value> aMap;
    aMap["dependencies"] = anArray;
    xmlRpcData = xmlrpc_c::value_struct(aMap);
}

xmlrpc_c::value WorkflowData::GetXmlRpcData()
{
    if (xmlRpcData.isInstantiated())
        return xmlRpcData;
    GenerateXmlRpcData();   
    return xmlRpcData;
}

xmlrpc_c::value WorkflowData::DumpXmlRpcDataRecursive(Dependency* D)
{
    map<string, xmlrpc_c::value> aMap;
    aMap["hop_urn"] = xmlrpc_c::value_string(D->GetHopUrn());
    aMap["aggregate_urn"] = xmlrpc_c::value_string(D->GetAggregateUrn());
    aMap["aggregate_url"] = xmlrpc_c::value_string(D->GetAggregateUrl());
    aMap["import_vlans"] = xmlrpc_c::value_boolean(D->isGetVlanFrom());

    if (!D->isLeaf()) 
    {
        vector<xmlrpc_c::value> arrayData;
        vector<Dependency*>::iterator itLower = D->GetLowers().begin();
        for (; itLower != D->GetLowers().end(); itLower++)
        {
            arrayData.push_back(DumpXmlRpcDataRecursive(*itLower));
        }
        xmlrpc_c::value_array anArray(arrayData);
        aMap["dependencies"] = anArray;
    }
    
    return xmlrpc_c::value_struct(aMap);
}