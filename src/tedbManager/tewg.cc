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

#include "exception.hh"
#include "log.hh"
#include "tewg.hh"


TDomain* TDomain::Clone(bool newSubLevels)
{
    TDomain* td = new TDomain(this->_id, this->name, this->address);
    td->disabled = this->disabled;
    map<string, Node*, strcmpless>::iterator itn = this->nodes.begin();
    for (; itn != this->nodes.end(); itn++)
        td->nodes[(*itn).first] = (newSubLevels ? ((TNode*)(*itn).second)->Clone(newSubLevels) : (*itn).second);
    return td;            
}


void TNode::AddLocalLink(TLink* link)
{
    if (HasLocalLink(link))
    {
        char buf[128];
        snprintf(buf, 128, "Node::AddLocalLink raises Excaption: local link %s:%s has already existed.", 
            link->GetPort()->GetName().c_str(), link->GetName().c_str());
        throw TEDBException(buf);
    }
    this->lclLinks.push_back(link);
}


void TNode::AddRemoteLink(TLink* link)
{
    if (!link->VerifyRemoteLink())
        throw TEDBException((char*)"TNode::AddRmoteLink raises Excaption: invalid remote link parenet port is 'null'.");
    else if (HasRemoteLink(link))
    {
        char buf[128];
        snprintf(buf, 128, "Node::AddRemoteLink raises Excaption: remote link %s:%s has already existed.", 
            link->GetPort()->GetName().c_str(), link->GetName().c_str());
        throw TEDBException(buf);
    }
    this->rmtLinks.push_back(link);
}


bool TNode::HasLocalLink(TLink* link)
{
    list<TLink*>::iterator itl;
    for (itl = lclLinks.begin(); itl != lclLinks.end(); itl++)
        if ( (*itl) == link)
            return true;
    return false;
}


bool TNode::HasRemoteLink(TLink* link)
{
    list<TLink*>::iterator itl;
    for (itl = rmtLinks.begin(); itl != rmtLinks.end(); itl++)
        if ( (*itl) == link)
            return true;
    return false;
}

TNode* TNode::Clone(bool newSubLevels)
{
    TNode* tn = new TNode(this->_id, this->name, this->address);
    tn->domain = this->domain;
    tn->disabled = this->disabled;
    tn->visited = this->visited;
    map<string, Port*, strcmpless>::iterator itp = this->ports.begin();
    for (; itp != this->ports.end(); itp++)
        tn->ports[(*itp).first] = (newSubLevels ? ((TPort*)(*itp).second)->Clone(newSubLevels) : (*itp).second);
    list<TLink*>::iterator itl = this->lclLinks.begin();
    for (; itl != this->lclLinks.end(); itl++)
    {
        TLink* tl = (*itl);
        if (tl->GetPort() == NULL && newSubLevels)
            tl = tl->Clone();
        tn->lclLinks.push_back(tl);
    }
    itl = this->rmtLinks.begin();
    for (; itl != this->rmtLinks.end(); itl++)
    {
        tn->rmtLinks.push_back(*itl);
    }
    return tn;
}


TPort* TPort::Clone(bool newSubLevels)
{
    TPort* tp = new TPort(this->_id, this->name, this->address);
    tp->node = this->node;
    tp->disabled = this->disabled;
    tp->visited = this->visited;
    tp->maxBandwidth = this->maxBandwidth;
    tp->maxReservableBandwidth = this->maxReservableBandwidth;
    tp->minReservableBandwidth = this->minReservableBandwidth;
    tp->bandwidthGranularity = this->minReservableBandwidth;
    for (int i = 0; i < 8; i++)
        tp->unreservedBandwidth[i] = this->unreservedBandwidth[i];
    map<string, Link*, strcmpless>::iterator itl = this->links.begin();
    for (; itl != this->links.end(); itl++)
        tp->links[(*itl).first] = (newSubLevels ? ((TLink*)(*itl).second)->Clone() : (*itl).second);
    // TODO: Clone TPoint when implemented
    return tp;
}


bool TLink::VerifyEdgeLink() 
{
    if (!edgeOnly || rmtEnd != NULL)
        return false;
    return (lclEnd != NULL && lclEnd->HasLocalLink(this));
    
}


bool TLink::VerifyRemoteLink() 
{
    return (remoteLink != NULL && remoteLink->GetRemoteLink() == this);
}


bool TLink::VerifyFullLink() 
{
    return (VerifyRemoteLink() && lclEnd->HasLocalLink(this) && lclEnd->HasRemoteLink((TLink*)remoteLink) && rmtEnd->HasLocalLink((TLink*)remoteLink) && rmtEnd->HasRemoteLink(this));
}


// assume all TE links have at least one ISCD. In most cases there is only one ISCD (the).
ISCD* TLink::GetTheISCD()
{
    assert(swCapDescriptors.size() > 0);
    return swCapDescriptors.front();
}


TLink* TLink::Clone()
{
    TLink* tl = new TLink(this->_id, this->name, this->address);
    tl->disabled = this->disabled;
    tl->visited = this->visited;
    tl->port = this->port;
    tl->metric = this->metric;
    tl->maxBandwidth = this->maxBandwidth;
    tl->maxReservableBandwidth = this->maxReservableBandwidth;
    tl->minReservableBandwidth = this->minReservableBandwidth;
    tl->bandwidthGranularity = this->minReservableBandwidth;
    tl->remoteLink = this->remoteLink;
    for (int i = 0; i < 8; i++)
        tl->unreservedBandwidth[i] = this->unreservedBandwidth[i];
    list<ISCD*>::iterator its = swCapDescriptors.begin();
    for (; its != swCapDescriptors.end(); its++)
        tl->swCapDescriptors.push_back((*its)->Duplicate());
    list<IACD>::iterator ita = swAdaptDescriptors.begin();
    for (; ita != swAdaptDescriptors.end(); ita++)
        tl->swAdaptDescriptors.push_back(*ita);
    list<Link*>::iterator itl = this->containerLinks.begin();
    for (; itl != this->containerLinks.end(); itl++)
        tl->containerLinks.push_back(*itl);
    itl = this->componentLinks.begin();
    for (; itl != this->componentLinks.end(); itl++)
        tl->componentLinks.push_back(*itl);
    return tl;
}



void TGraph::AddDomain(TDomain* domain)
{
    tDomains.push_back(domain);
}


void TGraph::AddNode(TDomain* domain, TNode* node)
{
    tNodes.push_back(node);
    domain->AddNode(node);
    node->SetDomain(domain);
}


void TGraph::AddPort(TNode* node, TPort* port)
{
    tPorts.push_back(port);
    node->AddPort(port);
    port->SetNode(node);
}


void TGraph::AddLink(TPort* port, TLink* link)
{
    tLinks.push_back(link);
    port->AddLink(link);
    link->SetPort(port);
    if (port->GetNode()) {
        ((TNode*)port->GetNode())->AddLocalLink(link);
        link->SetLocalEnd((TNode*)port->GetNode());
    }
}


void TGraph::AddLink(TNode* node, TLink* link)
{
    tLinks.push_back(link);
    link->SetPort(NULL);
    node->AddLocalLink(link);
    link->SetLocalEnd(node);
}


TGraph* TGraph::Clone()
{
    TGraph* tg = new TGraph(name);
    list<TDomain*>::iterator itd = this->tDomains.begin();
    for (; itd != this->tDomains.end(); itd++)
    {
        TDomain* td = (*itd)->Clone(true);
        tg->GetDomains().push_back(td);
        map<string, Node*, strcmpless>::iterator itn = td->GetNodes().begin();
        for (; itn != td->GetNodes().end(); itn++)
        {
            TNode* tn = (TNode*)(*itn).second;
            tg->GetNodes().push_back(tn);
            map<string, Port*, strcmpless>::iterator itp = tn->GetPorts().begin();
            for (; itp != tn->GetPorts().end(); itp++)
            {
                TPort* tp = (TPort*)(*itp).second;
                tg->GetPorts().push_back(tp);
                map<string, Link*, strcmpless>::iterator itl = tp->GetLinks().begin();
                for (; itl != tp->GetLinks().end(); itl++)
                    tg->GetLinks().push_back((TLink*)(*itl).second);
            }
            list<TLink*>::iterator itll = tn->GetLocalLinks().begin();
            for (; itll != tn->GetLocalLinks().end(); itll++)
            {
                TLink* tl = (*itll);
                if (tl->GetPort() == NULL)
                    tg->GetLinks().push_back(tl);
            }
        }
    }
    return tg;
}



void TGraph::LogDump()
{
    char buf[102400]; //up to 100K
    char str[128];
    strcpy(buf, "TEWG Dump...\n");
    list<TDomain*>::iterator itd = this->tDomains.begin();
    for (; itd != this->tDomains.end(); itd++)
    {
        TDomain* td = (*itd);
        snprintf(str, 128, "<domain id=%s>\n", td->GetName().c_str());
        strcat(buf, str);
        map<string, Node*, strcmpless>::iterator itn = td->GetNodes().begin();
        for (; itn != td->GetNodes().end(); itn++)
        {
            TNode* tn = (TNode*)(*itn).second;
            snprintf(str, 128, "\t<node id=%s>\n", tn->GetName().c_str());
            strcat(buf, str);
            map<string, Port*, strcmpless>::iterator itp = tn->GetPorts().begin();
            for (; itp != tn->GetPorts().end(); itp++)
            {
                TPort* tp = (TPort*)(*itp).second;
                snprintf(str, 128, "\t\t<port id=%s>\n", tp->GetName().c_str());
                strcat(buf, str);
                map<string, Link*, strcmpless>::iterator itl = tp->GetLinks().begin();
                for (; itl != tp->GetLinks().end(); itl++) 
                {
                    TLink* tl = (TLink*)(*itl).second;
                    snprintf(str, 128, "\t\t\t<link id=%s>\n", tl->GetName().c_str());
                    strcat(buf, str);
                    if (tl->GetRemoteLink())
                    {
                        snprintf(str, 128, "\t\t\t\t<remoteLinkId>domain=%s:node=%s:port=%s:link=%s</remoteLinkId>\n",  
                            tl->GetRemoteLink()->GetPort()->GetNode()->GetDomain()->GetName().c_str(),
                            tl->GetRemoteLink()->GetPort()->GetNode()->GetName().c_str(),
                            tl->GetRemoteLink()->GetPort()->GetName().c_str(), 
                            tl->GetRemoteLink()->GetName().c_str());
                        strcat(buf, str);
                    }
                    snprintf(str, 128, "\t\t\t</link>\n");
                    strcat(buf, str);
                }
                snprintf(str, 128, "\t\t</port>\n");
                strcat(buf, str);
            }
            snprintf(str, 128, "\t</node>\n");
            strcat(buf, str);
        }
        snprintf(str, 128, "</domain>\n");
        strcat(buf, str);
    }    
    LOG_DEBUG(buf);
}

