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

// TODO: Generate unique nodeID-number (for xml) ?

void DBDomain::UpdateToXML(bool populateSubLevels)
{
    //$$$$ loop-call node->UpdateToXML()
    //$$$$ update content of xmlElem from local variables
}


void DBDomain::UpdateFromXML(bool populateSubLevels)
{
    // update local variables from content of xmlElem
    gettimeofday (&this->updateTime, NULL);

    //match up node level elements
    assert(this->xmlElem->children);
    xmlNodePtr nodeLevel;
    for (nodeLevel = this->xmlElem->children; nodeLevel != NULL; nodeLevel = nodeLevel->next)
    {
        if (nodeLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)nodeLevel->name, "node", 4) == 0)
        {
            bool newNode = false;
            string nodeName = (const char*)xmlGetProp(nodeLevel, (const xmlChar*)"id");
            string aName = nodeName = (this->isNestedUrn()? GetUrnField(nodeName, "node") : nodeName);
            if (aName.length() > 0)
                nodeName = aName;
            DBNode* node = NULL;
            if (this->nodes.find(nodeName) != this->nodes.end())
            {
                node = (DBNode*)this->nodes[nodeName];
            }
            if (node == NULL)
            {
                node = new DBNode(tedb, 0, nodeName);
                node->SetXmlElement(nodeLevel);            
                node->SetDomain(this);
                nodes[nodeName] = node;
                newNode = true;
            }
            // populate XML update to node level
            if (populateSubLevels)
                node->UpdateFromXML(true);
        }
        else if (nodeLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)nodeLevel->name, "address", 7) == 0)
        {
            StripXmlString(this->address, xmlNodeGetContent(nodeLevel));
        }
        else if (nodeLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)nodeLevel->name, "capabilities", 10) == 0)
        {
            xmlNodePtr capLevel;
            for (capLevel = nodeLevel->children; capLevel != NULL; capLevel = capLevel->next)
            {
                if (capLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)capLevel->name, "capability", 10) == 0)
                {   
                    string capStr;
                    StripXmlString(capStr, xmlNodeGetContent(capLevel));
                    this->capabilities[capStr] = "true";
                }
            }
        }
        else if (nodeLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)nodeLevel->name, "isNestedUrn", 10) == 0)
        {
            string strIsNested;
            StripXmlString(strIsNested, xmlNodeGetContent(nodeLevel));
            if (strIsNested.compare("false") == 0)
            {
                this->nestedUrn = false;
            }
        }
        // TODO: parse NodeIfAdaptMatrix?
    }
    // cleanup nodes that no longer exist in XML
    map<string, Node*, strcmpless>::iterator itn = nodes.begin();
    while (itn != nodes.end())
    {
        if(((DBNode*)(*itn).second)->GetXmlElement() == NULL)
        {
            delete (*itn).second;
            map<string, Node*, strcmpless>::iterator toerase = itn;
            ++itn;
            nodes.erase(toerase);
        }
        else
            ++itn;
    }
}


TDomain* DBDomain::Checkout(TGraph* tg)
{
    TDomain* td = new TDomain(this->_id, this->name, this->address);
    td->setNestedUrn(this->nestedUrn);
    map<string, Node*, strcmpless>::iterator itn = this->nodes.begin();
    for (; itn != this->nodes.end(); itn++) 
    {
        TNode* tn = ((DBNode*)(*itn).second)->Checkout(tg);
        tg->AddNode(td, tn);
    }
    map<string, string, strcmpless>::iterator itc = this->capabilities.begin();
    for (; itc != this->capabilities.end(); itc++) 
    {
        td->GetCapabilities()[(*itc).first] = (*itc).second;
    }
    tg->AddDomain(td);
    return td;
}

DBDomain::~DBDomain()
{
    // delete sublevels
    map<string, Node*, strcmpless>::iterator itn = nodes.begin();
    for (; itn != nodes.end(); itn++)
    {
        delete (*itn).second;
    }
}


void DBNode::UpdateToXML(bool populateSubLevels)
{
    //$$$$ loop-call node->UpdateToXML()
    //$$$$ update content of xmlElem from local variables
}


void DBNode::UpdateFromXML(bool populateSubLevels)
{
    // update local variables from content of xmlElem
    gettimeofday (&this->updateTime, NULL);

    //match up node level elements
    assert(this->xmlElem->children);
    xmlNodePtr portLevel;
    for (portLevel = this->xmlElem->children; portLevel != NULL; portLevel = portLevel->next)
    {
        if (portLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)portLevel->name, "port", 4) == 0)
        {
            bool newPort = false;
            string portName = (const char*)xmlGetProp(portLevel, (const xmlChar*)"id");
            string aName = (this->GetDomain()->isNestedUrn() ? GetUrnField(portName, "port") : portName);
            if (aName.length() > 0)
                portName = aName;
            DBPort* port = NULL;
            if (this->ports.find(portName) != this->ports.end())
            {
                port = (DBPort*)this->ports[portName];
            }
            if (port == NULL)
            {
                port = new DBPort(tedb, 0, portName);
                port->SetXmlElement(portLevel);            
                port->SetNode(this);
                ports[portName] = port;
                newPort = true;
            }
            // populate XML update to node level
            if (populateSubLevels)
                port->UpdateFromXML(true);
        }
        else if (portLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)portLevel->name, "address", 7) == 0)
        {
           StripXmlString(this->address, xmlNodeGetContent(portLevel));
        }
        else if (portLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)portLevel->name, "capabilities", 10) == 0)
        {
            xmlNodePtr capLevel;
            for (capLevel = portLevel->children; capLevel != NULL; capLevel = capLevel->next)
            {
                if (capLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)capLevel->name, "capability", 10) == 0)
                {   
                    string capStr;
                    StripXmlString(capStr, xmlNodeGetContent(capLevel));
                    this->capabilities[capStr] = "true";
                }
            }
        }
    }
    // cleanup ports that no longer exist in XML
    
    map<string, Port*, strcmpless>::iterator itp = ports.begin();
    while (itp != ports.end())
    {
        if(((DBPort*)(*itp).second)->GetXmlElement() == NULL)
        {
            delete (*itp).second;
            map<string, Port*, strcmpless>::iterator toerase = itp;
            ++itp;
            ports.erase(toerase);
        }
        else
            ++itp;
    }
}


TNode* DBNode::Checkout(TGraph* tg)
{
    TNode* tn = new TNode(this->_id, this->name, this->address);
    map<string, Port*, strcmpless>::iterator itp = this->ports.begin();
    for (; itp != this->ports.end(); itp++)
    {
        TPort* tp = ((DBPort*)(*itp).second)->Checkout(tg);
        tg->AddPort(tn, tp);
        map<string, Link*, strcmpless>::iterator itl = tp->GetLinks().begin();
        for (; itl != tp->GetLinks().end(); itl++)
        {
            TLink* tl = (TLink*)(*itl).second;
            tg->AddLink(tn, tl);
            if (tl->GetRemoteLink()) 
            {
                ((TLink*)tl->GetRemoteLink())->SetRemoteEnd(tn);
                tl->SetRemoteEnd(((TLink*)tl->GetRemoteLink())->GetLocalEnd());
                ((TLink*)tl->GetRemoteLink())->SetRemoteEnd(tn);
            }
        }
        map<string, string, strcmpless>::iterator itc = this->capabilities.begin();
        for (; itc != this->capabilities.end(); itc++) 
        {
            tn->GetCapabilities()[(*itc).first] = (*itc).second;
        }
    }
    return tn;
}


DBNode::~DBNode()
{
    // delete sublevels
    map<string, Port*, strcmpless>::iterator itp = ports.begin();
    for (; itp != ports.end(); itp++)
    {
        delete (*itp).second;
    }
}


void DBPort::UpdateToXML(bool populateSubLevels)
{
    //$$$$ loop-call node->UpdateToXML()
    //$$$$ update content of xmlElem from local variables
}


void DBPort::UpdateFromXML(bool populateSubLevels)
{
    // update local variables from content of xmlElem
    gettimeofday (&this->updateTime, NULL);

    //match up node level elements
    assert(this->xmlElem->children);
    xmlNodePtr linkLevel;
    for (linkLevel = this->xmlElem->children; linkLevel != NULL; linkLevel = linkLevel->next)
    {
        if (linkLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)linkLevel->name, "link", 4) == 0)
        {
            bool newLink = false;
            string linkName = (const char*)xmlGetProp(linkLevel, (const xmlChar*)"id");
            string aName = (this->GetNode()->GetDomain()->isNestedUrn() ?GetUrnField(linkName, "link") : linkName);
            if (aName.length() > 0)
                linkName = aName;

            DBLink* link = NULL;
            if (this->links.find(linkName) != this->links.end())
            {
                link = (DBLink*)this->links[linkName];
            }
            if (link == NULL)
            {
                link = new DBLink(tedb, 0, linkName);
                link->SetXmlElement(linkLevel);
                link->SetPort(this);
                links[linkName] = link;
                newLink = true;
            }
            // populate XML update to node level
            if (populateSubLevels)
                link->UpdateFromXML();
        }
        else if (linkLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)linkLevel->name, "capacity", 8) == 0)
        {
            string bwStr;
            StripXmlString(bwStr, xmlNodeGetContent(linkLevel));
            this->maxBandwidth = StringToBandwidth(bwStr);
        }
        else if (linkLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)linkLevel->name, "maximumReservableCapacity", 17) == 0)
        {
            string bwStr;
            StripXmlString(bwStr, xmlNodeGetContent(linkLevel));
            this->maxReservableBandwidth= StringToBandwidth(bwStr);
            //?
            for (int i = 0; i < 8; i++)
                this->unreservedBandwidth[i] = this->maxReservableBandwidth;
        }
        else if (linkLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)linkLevel->name, "minimumReservableCapacity", 17) == 0)
        {
            string bwStr;
            StripXmlString(bwStr, xmlNodeGetContent(linkLevel));
            this->minReservableBandwidth= StringToBandwidth(bwStr);
        }
        else if (linkLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)linkLevel->name, "granularity", 10) == 0)
        {
            string bwStr;
            StripXmlString(bwStr, xmlNodeGetContent(linkLevel));
            this->bandwidthGranularity = StringToBandwidth(bwStr);
        }
        else if (linkLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)linkLevel->name, "capabilities", 10) == 0)
        {
            xmlNodePtr capLevel;
            for (capLevel = linkLevel->children; capLevel != NULL; capLevel = capLevel->next)
            {
                if (capLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)capLevel->name, "capability", 10) == 0)
                {   
                    string capStr;
                    StripXmlString(capStr, xmlNodeGetContent(capLevel));
                    this->capabilities[capStr] = "true";
                }
            }
        }
    }

    if (this->maxReservableBandwidth == 0) 
    {
        this->maxReservableBandwidth = this->maxBandwidth;
        for (int i = 0; i < 8; i++)
            this->unreservedBandwidth[i] = this->maxReservableBandwidth;
    }

    // cleanup links that no longer exist in XML
    map<string, Link*, strcmpless>::iterator itl = links.begin();
    while (itl != links.end())
    {
        if(((DBLink*)(*itl).second)->GetXmlElement() == NULL)
        {
            if ((*itl).second->GetRemoteLink() != NULL)
                (*itl).second->GetRemoteLink()->SetRemoteLink(NULL);
            delete (*itl).second;
            map<string, Link*, strcmpless>::iterator toerase = itl;
            ++itl;
            links.erase(toerase);
        }
        else
            ++itl;
    }
}


TPort* DBPort::Checkout(TGraph* tg)
{
    TPort* tp = new TPort(this->_id, this->name, this->address);
    tp->SetMaxBandwidth(this->maxBandwidth);
    tp->SetMaxReservableBandwidth(this->maxReservableBandwidth);
    for (int i = 0; i < 8; i++)
        tp->GetUnreservedBandwidth()[i] = this->unreservedBandwidth[i];
    tp->SetMinReservableBandwidth(this->minReservableBandwidth);
    tp->SetBandwidthGranularity(this->bandwidthGranularity);
    map<string, Link*, strcmpless>::iterator itl = this->links.begin();
    for (; itl != this->links.end(); itl++)
    {
        TLink* tl = ((DBLink*)(*itl).second)->Checkout(tg);
        tg->AddLink(tp, tl);
    }
    map<string, string, strcmpless>::iterator itc = this->capabilities.begin();
    for (; itc != this->capabilities.end(); itc++) 
    {
        tp->GetCapabilities()[(*itc).first] = (*itc).second;
    }
    return tp;
}


DBPort::~DBPort()
{
    // delete sublevels
    map<string, Link*, strcmpless>::iterator itl = links.begin();
    for (; itl != links.end(); itl++)
    {
        delete (*itl).second;
    }
}


void DBLink::UpdateToXML(bool populateSubLevels)
{
    //$$$$ loop-call node->UpdateToXML()
    //$$$$ update content of xmlElem from local variables
}


void DBLink::UpdateFromXML(bool populateSubLevels)
{
    // update local variables from content of xmlElem
    gettimeofday (&this->updateTime, NULL);

    //match up node level elements
    assert(this->xmlElem->children);
    swCapDescriptors.clear();
    xmlNodePtr sublinkLevel;
    for (sublinkLevel = this->xmlElem->children; sublinkLevel != NULL; sublinkLevel = sublinkLevel->next)
    {
        if (sublinkLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)sublinkLevel->name, "address", 8) == 0)
        {
            StripXmlString(this->address, xmlNodeGetContent(sublinkLevel));
        }
        if (sublinkLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)sublinkLevel->name, "capacity", 8) == 0)
        {
            string bwStr;
            StripXmlString(bwStr, xmlNodeGetContent(sublinkLevel));
            this->maxBandwidth = StringToBandwidth(bwStr);
        }
        else if (sublinkLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)sublinkLevel->name, "maximumReservableCapacity", 17) == 0)
        {
            string bwStr;
            StripXmlString(bwStr, xmlNodeGetContent(sublinkLevel));
            this->maxReservableBandwidth= StringToBandwidth(bwStr);
            //?
            for (int i = 0; i < 8; i++)
                this->unreservedBandwidth[i] = this->maxReservableBandwidth;
        }
        else if (sublinkLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)sublinkLevel->name, "minimumReservableCapacity", 17) == 0)
        {
            string bwStr;
            StripXmlString(bwStr, xmlNodeGetContent(sublinkLevel));
            this->minReservableBandwidth= StringToBandwidth(bwStr);
        }
        else if (sublinkLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)sublinkLevel->name, "granularity", 10) == 0)
        {
            string bwStr;
            StripXmlString(bwStr, xmlNodeGetContent(sublinkLevel));
            this->bandwidthGranularity = StringToBandwidth(bwStr);
        }
        else if (sublinkLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)sublinkLevel->name, "trafficEngineeringMetric", 10) == 0)
        {
            string metricStr;
            StripXmlString(metricStr, xmlNodeGetContent(sublinkLevel));
            sscanf(metricStr.c_str(), "%d", &this->metric);
        }
        else if (sublinkLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)sublinkLevel->name, "remoteLinkId", 10) == 0)
        {
            string rmtLinkStr;
            StripXmlString(rmtLinkStr, xmlNodeGetContent(sublinkLevel));
            DBLink* rmtLink = tedb->LookupLinkByURN(rmtLinkStr);
            if (rmtLink != NULL && rmtLink->GetRemoteLink() == NULL)
            {
                this->remoteLink = rmtLink;
                rmtLink->SetRemoteLink(this);
            }
        }
		// compatible with both switchingCapabilityDescriptors (NML old) and switchingCapabilityDescriptor (NML 20110826)
        else if (sublinkLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)sublinkLevel->name, "switchingCapabilityDescriptor", 29) == 0)
        {
            ISCD* iscd = GetISCDFromXML(sublinkLevel);
            if (iscd != NULL)
                swCapDescriptors.push_back(iscd); 
        }
        // adjustmentCapabilityDescriptor (NML 20110826)
        else if (sublinkLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)sublinkLevel->name, "adjustmentCapabilityDescriptor", 30) == 0)
        {
            IACD* iacd = GetIACDFromXML(sublinkLevel);
            if (iacd != NULL)
                this->adjCapDescriptors.push_back(iacd);
        }
        else if (sublinkLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)sublinkLevel->name, "capabilities", 10) == 0)
        {
            xmlNodePtr capLevel;
            for (capLevel = sublinkLevel->children; capLevel != NULL; capLevel = capLevel->next)
            {
                if (capLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)capLevel->name, "capability", 10) == 0)
                {   
                    string capStr;
                    StripXmlString(capStr, xmlNodeGetContent(capLevel));
                    this->capabilities[capStr] = "true";
                }
            }
        }

        // use info from port level if not found at link level
        if (this->port != NULL) 
        {
            if (this->maxBandwidth == 0)
                this->maxBandwidth = this->port->GetMaxBandwidth();
            if (this->maxReservableBandwidth == 0)
            {
                this->maxReservableBandwidth = this->port->GetMaxBandwidth();
                for (int i = 0; i < 8; i++)
                    this->unreservedBandwidth[i] = this->maxReservableBandwidth;
            }
            if (this->minReservableBandwidth == 0)
                this->minReservableBandwidth = this->port->GetMinReservableBandwidth();
            if (this->bandwidthGranularity == 0)
                this->bandwidthGranularity = this->port->GetBandwidthGranularity();
        }
            
    }
}


ISCD* DBLink::GetISCDFromXML(xmlNodePtr xmlNode)
{
    ISCD* iscd = NULL;
    u_char swType = 0, encType = 0;
    xmlNodePtr specLevel = NULL;
    xmlNodePtr specSubLevel = NULL;
    int mtu = 0;
    string vlanRange = "";
    bool vlanTranslation = false;
    u_int64_t capacity = 0;
    TDMConcatenationType concatenationType = STS1;
    string timeslotRange = "";
    bool tsiEnabled = true;
    bool vcatEnabled = true;
    WDMChannelRepresentationType channelRepresentation = ITU_GRID_50GHZ;
    string wavelengthRange = "";
    bool wavelengthConversion = false;
    xmlNodePtr vendorSpecInfo = NULL;
    for (xmlNode = xmlNode->children; xmlNode != NULL; xmlNode = xmlNode->next)
    {
        if (xmlNode->type == XML_ELEMENT_NODE && strncasecmp((const char*)xmlNode->name, "switchingcapType", 16) == 0)
        {
            string swTypeStr;
            StripXmlString(swTypeStr, xmlNodeGetContent(xmlNode));
            if (strncasecmp((const char*)swTypeStr.c_str(), "l2sc", 4) == 0)
                swType = LINK_IFSWCAP_L2SC;
            else if (strncasecmp((const char*)swTypeStr.c_str(), "psc", 3) == 0)
                swType = LINK_IFSWCAP_PSC1;
            else if (strncasecmp((const char*)swTypeStr.c_str(), "tdm", 4) == 0)
                swType = LINK_IFSWCAP_TDM;
            else if (strncasecmp((const char*)swTypeStr.c_str(), "lsc", 4) == 0)
                swType = LINK_IFSWCAP_LSC;
        }
        else if (xmlNode->type == XML_ELEMENT_NODE && strncasecmp((const char*)xmlNode->name, "encodingType", 12) == 0)
        {
            string encTypeStr;
            StripXmlString(encTypeStr, xmlNodeGetContent(xmlNode));
            if (strncasecmp((const char*)encTypeStr.c_str(), "ethernet", 6) == 0)
                encType = LINK_IFSWCAP_ENC_ETH;
            else if (strncasecmp((const char*)encTypeStr.c_str(), "packet", 6) == 0)
                encType = LINK_IFSWCAP_ENC_PKT;
            else if (strncasecmp((const char*)encTypeStr.c_str(), "sonet", 5) == 0 || strncasecmp((const char*)encTypeStr.c_str(), "sdh", 3) == 0)
                encType = LINK_IFSWCAP_ENC_SONETSDH;
            else if (strncasecmp((const char*)encTypeStr.c_str(), "lambda", 6) == 0)
                encType = LINK_IFSWCAP_ENC_LAMBDA;
        }
        else if (xmlNode->type == XML_ELEMENT_NODE && strncasecmp((const char*)xmlNode->name, "switchingCapabilitySpecificInfo", 30) == 0)
        {
            for (specLevel = xmlNode->children; specLevel != NULL; specLevel = specLevel->next)
            {
                //// compatible with old NML switchingCapabilitySpecificInfo parameters
                if (specLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)specLevel->name, "capacity", 8) == 0)
                {
                    string bwStr;
                    StripXmlString(bwStr, xmlNodeGetContent(specLevel));
                    capacity = StringToBandwidth(bwStr);
                }
                else if (specLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)specLevel->name, "interfaceMTU", 12) == 0)
                {
                    string mtuStr;
                    StripXmlString(mtuStr, xmlNodeGetContent(specLevel));
                    sscanf(mtuStr.c_str(), "%d", &mtu);
                }
                else if (specLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)specLevel->name, "vlanRangeAvailability", 12) == 0)
                {
                    StripXmlString(vlanRange, xmlNodeGetContent(specLevel));
                }
                else if (specLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)specLevel->name, "vlanTranslation", 12) == 0)
                {
                    string translationStr;
                    StripXmlString(translationStr, xmlNodeGetContent(specLevel));
                    if (strncasecmp(translationStr.c_str(), "true", 4) == 0)
                        vlanTranslation = true;
                    else
                        vlanTranslation = false;
                }

                //// switchingCapabilitySpecificInfo for NML 20110826 revision 
                // simplified handling for TDM and LSC layers
                // 1. switchingCapabilitySpecificInfo / switchingCapabilitySpecificInfo_L2sc / interfaceMTU & vlanRangeAvailability & vlanTranslation
                if (specLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)specLevel->name, "switchingCapabilitySpecificInfo_L2sc", 15) == 0)
                {
                    for (specSubLevel = specLevel->children; specSubLevel != NULL; specSubLevel = specSubLevel->next)
                    {
                        if (specSubLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)specSubLevel->name, "interfaceMTU", 17) == 0)
                        {
                            string mtuStr;
                            StripXmlString(mtuStr, xmlNodeGetContent(specSubLevel));
                            sscanf(mtuStr.c_str(), "%d", &mtu);
                        }
                        else if (specSubLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)specSubLevel->name, "vlanRangeAvailability", 16) == 0)
                        {
                            StripXmlString(vlanRange, xmlNodeGetContent(specSubLevel));
                        }
                        else if (specSubLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)specSubLevel->name, "vlanTranslation", 10) == 0)
                        {
                            string translationStr;
                            StripXmlString(translationStr, xmlNodeGetContent(specSubLevel));
                            if (strncasecmp(translationStr.c_str(), "true", 4) == 0)
                                vlanTranslation = true;
                            else
                                vlanTranslation = false;
                        }
                    }
                }
                else if (specLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)specLevel->name, "switchingCapabilitySpecificInfo_Tdm", 15) == 0)
                {
                    for (specSubLevel = specLevel->children; specSubLevel != NULL; specSubLevel = specSubLevel->next)
                    {
                        if (specSubLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)specSubLevel->name, "concatenationType", 17) == 0)
                        {
                            string typeStr;
                            StripXmlString(typeStr, xmlNodeGetContent(specSubLevel));
                            if (strncasecmp(typeStr.c_str(), "sts3c", 4) == 0 || strncasecmp(typeStr.c_str(), "150mbps", 4) == 0)
                                concatenationType = STS3C;
                            else
                                concatenationType = STS1;
                        }
                        else if (specSubLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)specSubLevel->name, "timeslotRangeSet", 16) == 0)
                        {
                            StripXmlString(timeslotRange, xmlNodeGetContent(specSubLevel));
                        }
                        else if (specSubLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)specSubLevel->name, "tsiEnabled", 10) == 0)
                        {
                            string enabledStr;
                            StripXmlString(enabledStr, xmlNodeGetContent(specSubLevel));
                            if (strncasecmp(enabledStr.c_str(), "true", 4) == 0)
                                tsiEnabled = true;
                            else
                                tsiEnabled = false;
                        }
                        else if (specSubLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)specSubLevel->name, "vcatEnabled", 10) == 0)
                        {
                            string enabledStr;
                            StripXmlString(enabledStr, xmlNodeGetContent(specSubLevel));
                            if (strncasecmp(enabledStr.c_str(), "true", 4) == 0)
                                vcatEnabled = true;
                            else
                                vcatEnabled = false;
                        }
                    }
                }
                // 2. switchingCapabilitySpecificInfo / switchingCapabilitySpecificInfo_Lsc /channelRepresentation & wavelengthRangeSet & wavelengthConversionEnabled
                else if (specLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)specLevel->name, "switchingCapabilitySpecificInfo_Lsc", 15) == 0)
                {
                    for (specSubLevel = specLevel->children; specSubLevel != NULL; specSubLevel = specSubLevel->next)
                    {
                        if (specSubLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)specSubLevel->name, "channelRepresentation", 20) == 0)
                        {
                            string typeStr;
                            StripXmlString(typeStr, xmlNodeGetContent(specSubLevel));
                            if (strncasecmp(typeStr.c_str(), "frequency-ghz", 3) == 0)
                                channelRepresentation = FREQUENCY_GHZ;
                            else if (strncasecmp(typeStr.c_str(), "wavelength-nm", 3) == 0)
                                channelRepresentation = WAVELENGTH_NM;
                            else if (strncasecmp(typeStr.c_str(), "itu-grid-100ghz", 12) == 0)
                                channelRepresentation = ITU_GRID_100GHZ;
                            else if (strncasecmp(typeStr.c_str(), "itu-grid-50ghz", 11) == 0)
                                channelRepresentation = ITU_GRID_50GHZ;
                        }
                        else if (specSubLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)specSubLevel->name, "wavelengthRangeSet", 18) == 0)
                        {
                            StripXmlString(wavelengthRange, xmlNodeGetContent(specSubLevel));
                        }
                        else if (specSubLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)specSubLevel->name, "wavelengthConversionEnabled", 25) == 0)
                        {
                            string translationStr;
                            StripXmlString(translationStr, xmlNodeGetContent(specSubLevel));
                            if (strncasecmp(translationStr.c_str(), "true", 4) == 0)
                                wavelengthConversion = true;
                            else
                                wavelengthConversion = false;
                        }

                    }
                }
                // 3. TODO switchingCapabilitySpecificInfo / switchingCapabilitySpecificInfo_Openflow 
                else if (specLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)specLevel->name, "switchingCapabilitySpecificInfo_Openflow", 15) == 0)
                {
                    //
                }
                // 4. Will we actually use the PSC, L2SC specific info in the sub-level? If so, add parsing here.
            }
        }        
        else if (xmlNode->type == XML_ELEMENT_NODE && strncasecmp((const char*)xmlNode->name, "vendorSpecificInfo", 18) == 0)
        {
            //// vendorSpecificInfo for NML 20110826 revision 
            // unified handling: xmlDocPointer + vendorString + specificDataParser (overriden)
            for (specLevel = xmlNode->children; specLevel != NULL; specLevel = specLevel->next)
            {
                // 1. vendorSpecificInfo / cienaOTNSpecificInfo
                if (specLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)specLevel->name, "cienaOTNSpecificInfo", 23) == 0)
                {
                    vendorSpecInfo = specLevel;
                }
                // 2. vendorSpecificInfo / infineraDTNSpecificInfo
                else if (specLevel->type == XML_ELEMENT_NODE && strncasecmp((const char*)specLevel->name, "infineraDTNSpecificInfo", 23) == 0)
                {
                    vendorSpecInfo = specLevel;
                }
            }
        }
    }
    switch (swType)
    {
        case LINK_IFSWCAP_L2SC:
            iscd = new ISCD_L2SC(capacity, mtu);
            ((ISCD_L2SC*)iscd)->availableVlanTags.LoadRangeString(vlanRange);
            ((ISCD_L2SC*)iscd)->vlanTranslation = vlanTranslation;
            break;
        case LINK_IFSWCAP_PSC1: //use PSC1 by default
            iscd = new ISCD_PSC(1, capacity, mtu);
            break;
        case LINK_IFSWCAP_PSC4: //PSC4 not used
            iscd = new ISCD_PSC(4, capacity, mtu);
            break;
        case LINK_IFSWCAP_TDM:
            iscd = new ISCD_TDM(capacity);
            ((ISCD_TDM*)iscd)->concatenationType = concatenationType;
            ((ISCD_TDM*)iscd)->availableTimeSlots.LoadRangeString(timeslotRange);
            ((ISCD_TDM*)iscd)->tsiEnabled = tsiEnabled;
            ((ISCD_TDM*)iscd)->vcatEnabled = vcatEnabled;
            break;
        case LINK_IFSWCAP_LSC:
            iscd = new ISCD_LSC(capacity);
            ((ISCD_LSC*)iscd)->channelRepresentation = channelRepresentation;
            if (channelRepresentation == ITU_GRID_50GHZ)// default
                ((ISCD_LSC*)iscd)->availableWavelengths.LoadRangeString_WaveGrid_50GHz(wavelengthRange);
            else
                ((ISCD_LSC*)iscd)->availableWavelengths.LoadRangeString(wavelengthRange);
            ((ISCD_LSC*)iscd)->wavelengthConversion = wavelengthConversion;
            break;
        default:
            //  default: L2SC / Ethernet
            swType = LINK_IFSWCAP_L2SC;
            encType = LINK_IFSWCAP_ENC_ETH;
            iscd = new ISCD_L2SC(capacity, mtu);
            ((ISCD_L2SC*)iscd)->availableVlanTags.LoadRangeString(vlanRange);
            ((ISCD_L2SC*)iscd)->vlanTranslation = vlanTranslation;
    }
    iscd->switchingType = swType;
    iscd->encodingType = encType;
    iscd->capacity = (capacity == 0 ? this->GetAvailableBandwidth() : capacity);
    iscd->vendorSpecInfoXml = vendorSpecInfo;
    return iscd;
}

IACD* DBLink::GetIACDFromXML(xmlNodePtr xmlNode)
{
    IACD* iacd = NULL;
    u_char swTypeLower = 0, encTypeLower = 0, swTypeUpper = 0, encTypeUpper = 0;
    u_int64_t capacity = 0;
    xmlNodePtr specLevel = NULL;
    for (xmlNode = xmlNode->children; xmlNode != NULL; xmlNode = xmlNode->next)
    {
        if (xmlNode->type == XML_ELEMENT_NODE && strncasecmp((const char*)xmlNode->name, "lowerSwcap", 10) == 0)
        {
            string swTypeStr;
            StripXmlString(swTypeStr, xmlNodeGetContent(xmlNode));
            if (strncasecmp((const char*)swTypeStr.c_str(), "l2sc", 4) == 0)
                swTypeLower = LINK_IFSWCAP_L2SC;
            else if (strncasecmp((const char*)swTypeStr.c_str(), "psc", 3) == 0)
                swTypeLower = LINK_IFSWCAP_PSC1;
            else if (strncasecmp((const char*)swTypeStr.c_str(), "tdm", 4) == 0)
                swTypeLower = LINK_IFSWCAP_TDM;
            else if (strncasecmp((const char*)swTypeStr.c_str(), "lsc", 4) == 0)
                swTypeLower = LINK_IFSWCAP_LSC;
        }
        else if (xmlNode->type == XML_ELEMENT_NODE && strncasecmp((const char*)xmlNode->name, "lowerEncType", 12) == 0)
        {
            string encTypeStr;
            StripXmlString(encTypeStr, xmlNodeGetContent(xmlNode));
            if (strncasecmp((const char*)encTypeStr.c_str(), "ethernet", 6) == 0)
                encTypeLower = LINK_IFSWCAP_ENC_ETH;
            else if (strncasecmp((const char*)encTypeStr.c_str(), "packet", 6) == 0)
                encTypeLower = LINK_IFSWCAP_ENC_PKT;
            else if (strncasecmp((const char*)encTypeStr.c_str(), "sonet", 5) == 0 || strncasecmp((const char*)encTypeStr.c_str(), "sdh", 3) == 0)
                encTypeLower = LINK_IFSWCAP_ENC_SONETSDH;
            else if (strncasecmp((const char*)encTypeStr.c_str(), "lambda", 6) == 0)
                encTypeLower = LINK_IFSWCAP_ENC_LAMBDA;
        }
        else if (xmlNode->type == XML_ELEMENT_NODE && strncasecmp((const char*)xmlNode->name, "upperSwcap", 10) == 0)
        {
            string swTypeStr;
            StripXmlString(swTypeStr, xmlNodeGetContent(xmlNode));
            if (strncasecmp((const char*)swTypeStr.c_str(), "l2sc", 4) == 0)
                swTypeUpper = LINK_IFSWCAP_L2SC;
            else if (strncasecmp((const char*)swTypeStr.c_str(), "psc", 3) == 0)
                swTypeUpper = LINK_IFSWCAP_PSC1;
            else if (strncasecmp((const char*)swTypeStr.c_str(), "tdm", 4) == 0)
                swTypeUpper = LINK_IFSWCAP_TDM;
            else if (strncasecmp((const char*)swTypeStr.c_str(), "lsc", 4) == 0)
                swTypeUpper = LINK_IFSWCAP_LSC;
        }
        else if (xmlNode->type == XML_ELEMENT_NODE && strncasecmp((const char*)xmlNode->name, "upperEncType", 12) == 0)
        {
            string encTypeStr;
            StripXmlString(encTypeStr, xmlNodeGetContent(xmlNode));
            if (strncasecmp((const char*)encTypeStr.c_str(), "ethernet", 6) == 0)
                encTypeUpper = LINK_IFSWCAP_ENC_ETH;
            else if (strncasecmp((const char*)encTypeStr.c_str(), "packet", 6) == 0)
                encTypeUpper = LINK_IFSWCAP_ENC_PKT;
            else if (strncasecmp((const char*)encTypeStr.c_str(), "sonet", 5) == 0 || strncasecmp((const char*)encTypeStr.c_str(), "sdh", 3) == 0)
                encTypeUpper = LINK_IFSWCAP_ENC_SONETSDH;
            else if (strncasecmp((const char*)encTypeStr.c_str(), "lambda", 6) == 0)
                encTypeUpper = LINK_IFSWCAP_ENC_LAMBDA;
        }
        else if (xmlNode->type == XML_ELEMENT_NODE && strncasecmp((const char*)xmlNode->name, "maximumAdjustableCapacity", 25) == 0)
        {
            string bwStr;
            StripXmlString(bwStr, xmlNodeGetContent(xmlNode));
            capacity = StringToBandwidth(bwStr);
        }
        else if (xmlNode->type == XML_ELEMENT_NODE && strncasecmp((const char*)xmlNode->name, "adjustmentCapabilitySpecificInfo", 30) == 0)
        {
            // TODO: create layer-specific IACD and fill in specific Info parameters
        }
    }
    if (iacd == NULL)
        iacd = new IACD(swTypeLower, encTypeLower, swTypeUpper, encTypeUpper, (capacity == 0 ? this->GetAvailableBandwidth() : capacity));
    return iacd;
}


TLink* DBLink::Checkout(TGraph* tg)
{
    TLink* tl = new TLink(this->_id, this->name, this->address);
    tl->SetMetric(this->metric);
    tl->SetMaxBandwidth(this->maxBandwidth);
    tl->SetMaxReservableBandwidth(this->maxReservableBandwidth);
    tl->SetMinReservableBandwidth(this->minReservableBandwidth);
    tl->SetBandwidthGranularity(this->bandwidthGranularity);
    for (int i = 0; i < 8; i++)
        tl->GetUnreservedBandwidth()[i] = this->unreservedBandwidth[i];
    list<ISCD*>::iterator its = this->GetSwCapDescriptors().begin();
    for (; its != this->GetSwCapDescriptors().end(); its++)
        tl->GetSwCapDescriptors().push_back((*its)->Duplicate());
    list<IACD*>::iterator ita = this->GetAdjCapDescriptors().begin();
    for (; ita != this->GetAdjCapDescriptors().end(); ita++)
        tl->GetAdjCapDescriptors().push_back(*ita);
    list<Link*>::iterator itl = this->GetContainerLinks().begin();
    //$$$$ update pointers to copy version?
    for (; itl != this->GetContainerLinks().end(); itl++)
        tl->GetContainerLinks().push_back(*itl);
    itl = this->GetComponentLinks().begin();
    //$$$$ update pointers to copy version?
    for (; itl != this->GetComponentLinks().end(); itl++)
        tl->GetComponentLinks().push_back(*itl);
    // correct remoteLink references
    tl->SetRemoteLink(this->remoteLink); // first point to the original remote link in TEDB
    list<TLink*>::iterator itl2 = tg->GetLinks().begin();
    for (; itl2 != tg->GetLinks().end(); itl2++)
    {
        TLink* tl2 = *itl2;
        // now get the real remote link in TEWG; only the later formed TLink in the pair has this
        if ((Link*)(tl2->GetRemoteLink()) == (Link*)this) 
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
    map<string, string, strcmpless>::iterator itc = this->capabilities.begin();
    for (; itc != this->capabilities.end(); itc++) 
    {
        tl->GetCapabilities()[(*itc).first] = (*itc).second;
    }
    return tl;
}


DBLink::~DBLink()
{
    for (list<ISCD*>::iterator it = swCapDescriptors.begin(); it != swCapDescriptors.end(); it++)
        delete (*it);
}

string TEDB::GetXmlTreeDomainName(xmlDocPtr xmlTree)
{
    string domainName = "";
    xmlNodePtr rootLevel;
    xmlNodePtr domainLevel;
    rootLevel = xmlDocGetRootElement(xmlTree);
    if (rootLevel->type != XML_ELEMENT_NODE || strncasecmp((const char*)rootLevel->name, "topology", 8) != 0)
    {
        throw TEDBException((char*)"TEDB::PopulateXmlTree failed to locate root <topology> element");
    }

    for (domainLevel = rootLevel->children; domainLevel != NULL; domainLevel = domainLevel->next)
    {
        if (domainLevel->type != XML_ELEMENT_NODE || strncasecmp((const char*)domainLevel->name, "domain", 6) != 0)
            continue;
        // get first domain if multiple
        domainName = (const char*)xmlGetProp(domainLevel, (const xmlChar*)"id");
        return domainName;
    }
    return domainName;
}

void TEDB::AddXmlDomainTree(xmlDocPtr xmlTree)
{
    string newDomainName = GetXmlTreeDomainName(xmlTree);
    list<xmlDocPtr>::iterator itX = xmlDomainTrees.begin();
    for (; itX != xmlDomainTrees.end(); itX++)
    {
        if (GetXmlTreeDomainName(*itX) == newDomainName)
        {
            xmlFreeDoc(*itX);
            itX = xmlDomainTrees.erase(itX);
            break;
        }
    }
    xmlDomainTrees.push_back(xmlTree);
}

void TEDB::ClearXmlTrees()
{
    list<DBDomain*>::iterator itd = dbDomains.begin();
    for (; itd != dbDomains.end(); itd++)
    {
        (*itd)->SetXmlElement(NULL);
    }
    list<DBNode*>::iterator itn = dbNodes.begin();
    for (; itn != dbNodes.end(); itn++)
    {
        (*itn)->SetXmlElement(NULL);
    }
    list<DBPort*>::iterator itp = dbPorts.begin();
    for (; itp != dbPorts.end(); itp++)
    {
        (*itp)->SetXmlElement(NULL);
    }    
    list<DBLink*>::iterator itl = dbLinks.begin();
    for (; itl != dbLinks.end(); itl++)
    {
        (*itl)->SetXmlElement(NULL);
    }    
}


void TEDB::PopulateXmlTrees()
{
    assert(xmlDomainTrees.size() > 0);

    // cleanup everything 
    list<DBDomain*>::iterator itd = dbDomains.begin();
    for (; itd != dbDomains.end(); itd++) 
    {
        (*itd)->GetNodes().clear();
    }
    // clean up dbNodes, dbPorts and dbLinks lists
    list<DBNode*>::iterator itn = dbNodes.begin();
    for (; itn != dbNodes.end(); itn++) 
    {
        delete (*itn);
    }
    dbNodes.clear();
    list<DBPort*>::iterator itp = dbPorts.begin();
    for (; itp != dbPorts.end(); itp++) 
    {
        delete (*itp);
    }
    dbPorts.clear();
    list<DBLink*>::iterator itl = dbLinks.begin();
    for (; itl != dbLinks.end(); itl++) 
    {
        // clean up ISAD, ISCD lists
        DBLink* L = (*itl);
        list<ISCD*>::iterator itcd1 = L->GetSwCapDescriptors().begin();
        for (; itcd1 != L->GetSwCapDescriptors().end(); itcd1++) 
        {
            delete (*itcd1);
        }
        list<IACD*>::iterator itcd2 = L->GetAdjCapDescriptors().begin();
        for (; itcd2 != L->GetAdjCapDescriptors().end(); itcd2++) 
        {
            delete (*itcd2);
        }
        delete L;
    }
    dbLinks.clear();

    // update topologies into TEDB
    xmlNodePtr node;
    xmlNodePtr rootLevel;
    xmlNodePtr domainLevel;
    xmlDocPtr xmlTree;
    list<xmlDocPtr>::iterator itX = xmlDomainTrees.begin();
    for (; itX != xmlDomainTrees.end(); itX++) {
        xmlTree = *itX;
        rootLevel = xmlDocGetRootElement(xmlTree);
        if (rootLevel->type != XML_ELEMENT_NODE || strncasecmp((const char*) rootLevel->name, "topology", 8) != 0) {
            throw TEDBException((char*) "TEDB::PopulateXmlTree failed to locate root <topology> element");
        }

        //update Domain level elements
        for (domainLevel = rootLevel->children; domainLevel != NULL; domainLevel = domainLevel->next)
        {
            if (domainLevel->type != XML_ELEMENT_NODE || strncasecmp((const char*)domainLevel->name, "domain", 6) != 0)
                continue;
            bool newDomain = false;
            string domainName = (const char*)xmlGetProp(domainLevel, (const xmlChar*)"id");
            string aName = GetUrnField(domainName, "domain");
            if (aName.length() > 0)
                domainName = aName;
            DBDomain* domain = LookupDomainByName(domainName);
            if (domain == NULL)
            {
                domain = new DBDomain(this, 0, domainName);
                dbDomains.push_back(domain);
                newDomain = true;
            }
            domain->SetXmlElement(domainLevel);            
            domain->UpdateFromXML(true);
        }

        // remove domains that no longer exist in XML
        list<DBDomain*>::iterator itd = dbDomains.begin();
        for (; itd != dbDomains.end(); itd++)
        {
            if((*itd)->GetXmlElement() == NULL)
            {
                delete (*itd);
                itd = dbDomains.erase(itd);
            }
        }

        // add back updated elements from dbDomains to the dbNodes, dbPorts and dbLinks lists
        for (itd = dbDomains.begin(); itd != dbDomains.end(); itd++)
        {
            map<string, Node*, strcmpless>::iterator itn = (*itd)->GetNodes().begin();
            for (; itn != (*itd)->GetNodes().end(); itn++)
            {
                dbNodes.push_back((DBNode*)(*itn).second);
                map<string, Port*, strcmpless>::iterator itp = (*itn).second->GetPorts().begin();
                for (; itp != (*itn).second->GetPorts().end(); itp++)
                {
                    dbPorts.push_back((DBPort*)(*itp).second);
                    map<string, Link*, strcmpless>::iterator itl = (*itp).second->GetLinks().begin();
                    for (; itl != (*itp).second->GetLinks().end(); itl++)
                    {
                        dbLinks.push_back((DBLink*)(*itl).second);
                    }
                }

            }
        }
    }
}

TEWG* TEDB::GetSnapshot(string& name)
{
    TEWG* tg = new TEWG(name);
    if (dbDomains.empty())
        return NULL;

    list<DBDomain*>::iterator itd = dbDomains.begin();
    for (; itd != dbDomains.end(); itd++)
        (*itd)->Checkout(tg);

    return tg;
}


DBDomain* TEDB::LookupDomainByName(string& name)
{
    list<DBDomain*>::iterator itd = dbDomains.begin();
    for (; itd != dbDomains.end(); itd++)
    {
        DBDomain* dbd = *itd;
        if (strcasecmp(dbd->GetName().c_str(), name.c_str()) == 0)
            return dbd;
    }
    return NULL;
}



DBDomain* TEDB::LookupDomainByURN(string& urn)
{
    string domainName = GetUrnField(urn, "domain");
    if (domainName.empty())
        return NULL;
    return LookupDomainByName(domainName);
}


DBNode* TEDB::LookupNodeByURN(string& urn)
{
    DBDomain* dbd = LookupDomainByURN(urn);
    if (dbd == NULL)
        return NULL;
    string nodeName = (dbd->isNestedUrn() ? GetUrnField(urn, "node") : urn);
    map<string, Node*, strcmpless>::iterator itn = dbd->GetNodes().find(nodeName);
    if (itn == dbd->GetNodes().end())
        return NULL;
    return (DBNode*)(*itn).second;
}


DBPort* TEDB::LookupPortByURN(string& urn)
{
    DBNode* dbn = LookupNodeByURN(urn);
    if (dbn == NULL)
        return NULL;
    string portName = (dbn->GetDomain()->isNestedUrn() ? GetUrnField(urn, "port") : urn);
    map<string, Port*, strcmpless>::iterator itp = dbn->GetPorts().find(portName);
    if (itp == dbn->GetPorts().end())
        return NULL;
    return (DBPort*)(*itp).second;
}


DBLink* TEDB::LookupLinkByURN(string& urn)
{
    DBDomain* dbd = LookupDomainByURN(urn);
    if (dbd == NULL)
        return NULL;
    if (!dbd->isNestedUrn())
    {
        map<string, Node*, strcmpless>::iterator itn = dbd->GetNodes().begin();
        for (; itn != dbd->GetNodes().end(); itn++) {
            map<string, Port*, strcmpless>::iterator itp = (*itn).second->GetPorts().begin();
            for (; itp != (*itn).second->GetPorts().end(); itp++) {
                map<string, Link*, strcmpless>::iterator itl = (*itp).second->GetLinks().find(urn);
                if (itl != (*itp).second->GetLinks().end())
                    return (DBLink*)(*itl).second;
            }
        }
    }    
    
    DBPort* dbp = LookupPortByURN(urn);
    if (dbp == NULL)
        return NULL;
    string linkName = GetUrnField(urn, "link");
    map<string, Link*, strcmpless>::iterator itl = dbp->GetLinks().find(linkName);
    if (itl == dbp->GetLinks().end())
        return NULL;
    return (DBLink*)(*itl).second;
}



void TEDB::LogDump()
{
    char buf[1024000]; //up to 1000K
    char str[1024];
    
    int nD = dbDomains.size();
    int nN = dbNodes.size();
    
    strcpy(buf, "TEDB Dump...\n");
    list<DBDomain*>::iterator itd = this->dbDomains.begin();
    for (; itd != this->dbDomains.end(); itd++)
    {
        DBDomain* td = (*itd);
        snprintf(str, 1024, "<domain id=%s>\n", td->GetName().c_str());
        strcat(buf, str);
        map<string, Node*, strcmpless>::iterator itn = td->GetNodes().begin();
        for (; itn != td->GetNodes().end(); itn++)
        {
            DBNode* tn = (DBNode*)(*itn).second;
            snprintf(str, 1024, "\t<node id=%s>\n", tn->GetName().c_str());
            strcat(buf, str);
            map<string, Port*, strcmpless>::iterator itp = tn->GetPorts().begin();
            for (; itp != tn->GetPorts().end(); itp++)
            {
                DBPort* tp = (DBPort*)(*itp).second;
                snprintf(str, 1024, "\t\t<port id=%s>\n", tp->GetName().c_str());
                strcat(buf, str);
                map<string, Link*, strcmpless>::iterator itl = tp->GetLinks().begin();
                for (; itl != tp->GetLinks().end(); itl++) 
                {
                    DBLink* tl = (DBLink*)(*itl).second;
                    snprintf(str, 1024, "\t\t\t<link id=%s>\n", tl->GetName().c_str());
                    strcat(buf, str);
                    if (tl->GetRemoteLink())
                    {
                        snprintf(str, 1024, "\t\t\t\t<remoteLinkId>domain=%s:node=%s:port=%s:link=%s</remoteLinkId>\n",  
                            tl->GetRemoteLink()->GetPort()->GetNode()->GetDomain()->GetName().c_str(),
                            tl->GetRemoteLink()->GetPort()->GetNode()->GetName().c_str(),
                            tl->GetRemoteLink()->GetPort()->GetName().c_str(), 
                            tl->GetRemoteLink()->GetName().c_str());
                        strcat(buf, str);
                    }
                    snprintf(str, 1024, "\t\t\t\t<MaxBandwidth>%llu</MaxBandwidth>\n", tl->GetMaxBandwidth());
                    strcat(buf, str);
                    snprintf(str, 1024, "\t\t\t\t<MaxReservableBandwidth>%llu</MaxReservableBandwidth>\n", tl->GetMaxReservableBandwidth());
                    strcat(buf, str);
                    snprintf(str, 1024, "\t\t\t\t<Granularity>%llu</Granularity>\n", tl->GetBandwidthGranularity());
                    strcat(buf, str);
                    snprintf(str, 1024, "\t\t\t</link>\n");
                    strcat(buf, str);
                }
                snprintf(str, 1024, "\t\t</port>\n");
                strcat(buf, str);
            }
            snprintf(str, 1024, "\t</node>\n");
            strcat(buf, str);
        }
        snprintf(str, 1024, "</domain>\n");
        strcat(buf, str);
    }    
    LOG_DEBUG(buf);
}


