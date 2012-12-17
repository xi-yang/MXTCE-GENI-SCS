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

void WorkflowData::LoadPath(TPath* tp)
{
    list<TLink*>::iterator itp = tp->GetPath().begin();
    for (; itp != tp->GetPath().end(); itp++)
    {
        TLink* L = *itp;
        Dependency* D = new Dependency();
        D->SetHopUrn(L->GetName());
        //D->SetAggregateUrn(?); //  stored in <capabilities>?
        //D->SetAggregateUrl(?); //  stored in <capabilities>?
        D->SetResourceRef(L);
    }
}

void WorkflowData::ComputeDependency()
{
    // 1. Create producing-consuming dependency relationships
    // 2. Create continuous VLAN dependency relationships
        // skip if causing loop
    // 3. Create VLAN translation dependency relationships
        // skip if causing loop

}

// generating a 'struct' member whose value is an array of 'dependencies'
void WorkflowData::GenerateXmlRpcData()
{
    if (dependencies.empty())
        return;
    char buf[1024];
    snprintf(buf, 1024, "<member><name>dependencies</name><value><array><data>");
    xmlRpcData += buf;
    vector<Dependency*>::iterator itD = dependencies.begin();
    for (; itD != dependencies.end(); itD++)
    {
        // dump itD
        if ((*itD)->isRoot()) 
        {
            xmlRpcData += DumpXmlRpcDataRecursive(*itD);
        }
    }
    snprintf(buf, 1024, "</data></array></value></member>");
    xmlRpcData += buf;    
    
}

string& WorkflowData::GetXmlRpcData()
{
    if (!xmlRpcData.empty())
        return xmlRpcData;
    GenerateXmlRpcData();   
    return xmlRpcData;
}

string WorkflowData::DumpXmlRpcDataRecursive(Dependency* D)
{
    string xml = "";
    char buf[1024];
    snprintf(buf, 1024, "<value><struct>"
            "<member><name>hop_urn</name><value><string>%s</string></value></member>"
            "<member><name>aggregate_urn</name><value><string>%s</string></value></member>"
            "<member><name>aggregate_url</name><value><string>%s</string></value></member>"
            "<member><name>get_vlan_from</name><value><boolean>%d</boolean></value></member>", 
            D->GetHopUrn().c_str(), D->GetAggregateUrn().c_str(), 
            D->GetAggregateUrl().c_str(), (int)D->isGetVlanFrom());
    xml += buf;
    if (!D->isLeaf()) 
    {
        snprintf(buf, 1024, "<member><name>dependencies</name><value><array><data>");
        xml += buf;
        vector<Dependency*>::iterator itLower = D->GetLowers().begin();
        for (; itLower != D->GetLowers().end(); itLower++)
        {
            xml += DumpXmlRpcDataRecursive(*itLower);
        }
        snprintf(buf, 1024, "</data></array></value></member>");
        xml += buf;
    }
    snprintf(buf, 1024, "</struct></value>");
    xml += buf;
}