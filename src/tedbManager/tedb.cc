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

#include "tedb.hh"
#include "exception.hh"


void DBDomain::UpdateToXML(bool populateSubLevels)
{
    //$$$$ loop-call node->UpdateToXML()
    //$$$$ update content of xmlElem from local variables
}


void DBDomain::UpdateFromXML(bool populateSubLevels)
{
    //$$$$ if(populateSubLevels)  loop-call node->UpdateFromXML()
    //$$$$ update local variables from content of xmlElem
}


TDomain* DBDomain::Checkout(TGraph* tg)
{
    TDomain* td = new TDomain(this->_id, this->name, this->address);
    map<string, Node*, strcmpless>::iterator itn = this->nodes.begin();
    for (; itn != this->nodes.end(); itn++) 
    {
        TNode* tn = ((DBNode*)(*itn).second)->Checkout(tg);
        tg->AddNode(td, tn);
    }
    tg->AddDomain(td);
    return td;
}


void DBNode::UpdateToXML(bool populateSubLevels)
{
    //$$$$ loop-call node->UpdateToXML()
    //$$$$ update content of xmlElem from local variables
}


void DBNode::UpdateFromXML(bool populateSubLevels)
{
    //$$$$ if(populateSubLevels)  loop-call node->UpdateFromXML()
    //$$$$ update local variables from content of xmlElem
}


TNode* DBNode::Checkout(TGraph* tg)
{
    TNode* tn = new TNode(this->_id, this->name, this->address);
    map<string, Port*, strcmpless>::iterator itp = this->ports.begin();
    for (; itp != this->ports.end(); itp++)
    {
        TPort* tp = ((DBPort*)(*itp).second)->Checkout(tg);
        tg->AddPort(tn, tp);
    }
    return tn;
}


void DBPort::UpdateToXML(bool populateSubLevels)
{
    //$$$$ loop-call node->UpdateToXML()
    //$$$$ update content of xmlElem from local variables
}


void DBPort::UpdateFromXML(bool populateSubLevels)
{
    //$$$$ if(populateSubLevels)  loop-call node->UpdateFromXML()
    //$$$$ update local variables from content of xmlElem
}


TPort* DBPort::Checkout(TGraph* tg)
{
    TPort* tp = new TPort(this->_id, this->name, this->address);
    tp->SetMaxBandwidth(this->maxBandwidth);
    tp->SetMaxReservableBandwidth(this->maxReservableBandwidth);
    for (int i = 0; i < 8; i++)
        tp->GetUnreservedBandwidth()[i] = this->unreservedBandwidth[i];
    map<string, Link*, strcmpless>::iterator itl = this->links.begin();
    for (; itl != this->links.end(); itl++)
    {
        TLink* tl = ((DBLink*)(*itl).second)->Checkout(tg);
        tg->AddLink(tp, tl);
    }
    return tp;
}


void DBLink::UpdateToXML(bool populateSubLevels)
{
    //$$$$ loop-call node->UpdateToXML()
    //$$$$ update content of xmlElem from local variables
}


void DBLink::UpdateFromXML(bool populateSubLevels)
{
    //$$$$ if(populateSubLevels)  loop-call node->UpdateFromXML()
    //$$$$ update local variables from content of xmlElem
}


TLink* DBLink::Checkout(TGraph* tg)
{
    TLink* tl = new TLink(this->_id, this->name, this->address);
    tl->SetMetric(this->metric);
    tl->SetMaxBandwidth(this->maxBandwidth);
    tl->SetMaxReservableBandwidth(this->maxReservableBandwidth);
    tl->SetMinReservableBandwidth(this->minReservableBandwidth);
    for (int i = 0; i < 8; i++)
        tl->GetUnreservedBandwidth()[i] = this->unreservedBandwidth[i];
    list<ISCD>::iterator its = this->GetSwCapDescriptors().begin();
    for (; its != this->GetSwCapDescriptors().end(); its++)
        tl->GetSwCapDescriptors().push_back(*its);
    list<IACD>::iterator ita = this->GetSwAdaptDescriptors().begin();
    for (; ita != this->GetSwAdaptDescriptors().end(); ita++)
        tl->GetSwAdaptDescriptors().push_back(*ita);
    list<Link*>::iterator itl = this->GetContainerLinks().begin();
    for (; itl != this->GetContainerLinks().end(); itl++)
        tl->GetContainerLinks().push_back(*itl);
    itl = this->GetComponentLinks().begin();
    for (; itl != this->GetComponentLinks().end(); itl++)
        tl->GetComponentLinks().push_back(*itl);
    // correct remoteLink references
    tl->SetRemoteLink(this->remoteLink);
    list<TLink*>::iterator itl2 = tg->GetLinks().begin();
    for (; itl2 != tg->GetLinks().end(); itl2++)
    {
        TLink* tl2 = *itl2;
        if ((Link*)tl2 == (Link*)this)
        {
            tl->SetRemoteLink(tl2);
            tl2->SetRemoteLink(tl);
            if (tl->GetPort() && tl->GetPort()->GetNode())
                ((TNode*)tl->GetPort()->GetNode())->AddRemoteLink(tl2);
            if (tl2->GetPort() && tl2->GetPort()->GetNode())
                ((TNode*)tl2->GetPort()->GetNode())->AddRemoteLink(tl);
            break;
        }
    }
    return tl;
}

TGraph* TEDB::CheckoutTEWG(string& name)
{
    TGraph* tg = new TGraph(name);
    if (dbDomains.empty())
        return NULL;

    list<DBDomain*>::iterator itd = dbDomains.begin();
    for (; itd != dbDomains.end(); itd++)
        (*itd)->Checkout(tg);

    return tg;
}

