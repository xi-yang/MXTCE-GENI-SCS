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

#include "resource.hh"
#include "exception.hh"
#include "reservation.hh"


void Resource::AddDelta(TDelta* delta) 
{ 
    deltaList.push_back(delta); delta->SetTargetResource(this); 
}

void Resource::RemoveDelta(TDelta* delta) 
{
    list<TDelta*>::iterator itd = deltaList.begin();
    for ( ; itd != deltaList.end(); itd++)
        if ((*itd) == delta)
            itd = deltaList.erase(itd);
}

void Resource::RemoveDeltasByName(string resvName) 
{
    list<TDelta*>::iterator itd = deltaList.begin();
    for ( ; itd != deltaList.end(); itd++)
        if ((*itd)->GetReservationName() == resvName)
            itd = deltaList.erase(itd);
}

list<TDelta*> Resource::LookupDeltasByName(string resvName) 
{
    list<TDelta*> deltaListNew;
    list<TDelta*>::iterator itd = deltaList.begin();
    for ( ; itd != deltaList.end(); itd++)
        if ((*itd)->GetReservationName() == resvName)
            deltaListNew.push_back(*itd);
    return deltaListNew;
}

VendorSpecificInfoParser * ISCD::VendorSpecificInfo() 
{
    if (vendorSpecInfoParser != NULL)
        return vendorSpecInfoParser;
    if (vendorSpecInfoXml != NULL) 
    {
        VendorSpecificInfoParser* parser = VendorSpecificInfoParserFactory::CreateParser(vendorSpecInfoXml);
        if (parser == NULL)
            throw TEDBException((char*)"VendorSpecificInfo for VendorSpecificInfoParserFactory - cannot create a parser.");
        parser->Parse();
        return parser;
    }
    return NULL;
}

void Domain::AddNode(Node* node)
{
    if (node->GetName().size() == 0)
        throw TEDBException((char*)"Domain::AddNode raises Excaption: node name is empty.");
    else if (nodes.find(node->GetName()) != nodes.end())
    {
        char buf[128];
        snprintf(buf, 128, "Domain::AddNode raises Excaption: node name %s has already existed.", node->GetName().c_str());
        throw TEDBException(buf);
    }
    this->nodes[node->GetName()] = node;
}


Node::~Node()
{
    if (ifAdaptMatrix)
        delete ifAdaptMatrix;
}

void Node::AddPort(Port* port)
{
    if (port->GetName().size() == 0)
        throw TEDBException((char*)"Node::AddPort raises Excaption: port name is empty.");
    else if (ports.find(port->GetName()) != ports.end())
    {
        char buf[128];
        snprintf(buf, 128, "Node::AddPort raises Excaption: port name %s has already existed.", port->GetName().c_str());
        throw TEDBException(buf);
    }
    this->ports[port->GetName()] = port;
}


void Port::AddLink(Link* link)
{
    link->SetPort(this);

    if (links.find(link->GetName()) != links.end())
    {
        char buf[128];
        snprintf(buf, 128, "Port::AddLink raises Excaption: link name %s has already existed.", link->GetName().c_str());
        throw TEDBException(buf);
    }
    this->links[link->GetName()] = link;
}


list<Port*> NodeIfAdaptMatrix::GetAdaptToPorts(Port* fromPort, u_int64_t bw)
{
    list<Port*> portList;
    if (portMap.find(fromPort) != portMap.end())
    {
        map<Port*, int, portcmpless>::iterator it;
        for (it = portMap.begin(); it != portMap.end(); ++it) 
        {  
            Port* toPort = (*it).first;
            if (toPort == fromPort)
                continue;
            if (GetAdaptCap(fromPort,toPort) >= bw)
                portList.push_back(toPort);
        }        
    }
    return portList;
}


list<Port*> NodeIfAdaptMatrix::GetAdaptFromPorts(Port* toPort, u_int64_t bw)
{
    list<Port*> portList;
    if (portMap.find(toPort) != portMap.end())
    {
        map<Port*, int, portcmpless>::iterator it;
        for (it = portMap.begin(); it != portMap.end(); ++it) 
        {  
            Port* fromPort = (*it).first;
            if (toPort == fromPort)
                continue;
            if (GetAdaptCap(fromPort,toPort) >= bw)
                portList.push_back(fromPort);
        }        
    }
    return portList;
}


