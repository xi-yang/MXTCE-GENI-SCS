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


#include "rspec.hh"
#include "tedb.hh"
#include "message.hh"

void GeniRSpec::ParseRspecXml()
{
    rspecDoc = xmlParseMemory(rspecXml.c_str(), rspecXml.size());
    if (rspecDoc == NULL)
    {
        char buf[1024*64];
        snprintf(buf, 1024*16, "GeniRSpec::ParseXML - Failed to parse RSpec XML string: %s", rspecXml.c_str());
        throw TEDBException(buf);
    }
}

void GeniRSpec::DumpRspecXml()
{
    if (rspecDoc == NULL)
    {
        throw TEDBException((char*)"GeniRSpec::DumpXml - Failed to dump RSpec XML: null rspecDoc");
    }
    xmlChar *xmlBuf;
    int sizeBuf;
    xmlDocDumpMemory(rspecDoc, &xmlBuf, &sizeBuf);
    rspecXml = (const char*)xmlBuf;
}

static string defaultSwcapStr = "<SwitchingCapabilityDescriptors>\
        <switchingcapType>l2sc</switchingcapType>\
        <encodingType>ethernet</encodingType>\
        <switchingCapabilitySpecificInfo>\
           <interfaceMTU>9000</interfaceMTU>\
           <vlanRangeAvailability>2-4094</vlanRangeAvailability>\
           <vlanTranslation>false</vlanTranslation>\
        </switchingCapabilitySpecificInfo>\
      </SwitchingCapabilityDescriptors>";

xmlDocPtr GeniAdRSpec::TranslateToNML()
{
    if (rspecDoc == NULL)
        return NULL;

    char buf[1024*1024];
    //get domain info: rspec/stitching/aggregate
    xmlNodePtr rspecRoot = xmlDocGetRootElement(rspecDoc);
    xmlNodePtr xmlNode;
    xmlNodePtr aggrNode = NULL;
    for (xmlNode = rspecRoot->children; xmlNode != NULL; xmlNode = xmlNode->next)
    {
        if (xmlNode->type == XML_ELEMENT_NODE )
        {
            if (strncasecmp((const char*)xmlNode->name, "stitching", 9) == 0) 
            {
                for (aggrNode = xmlNode->children; aggrNode != NULL; aggrNode = aggrNode->next)
                {
                    if (aggrNode->type == XML_ELEMENT_NODE )
                    {
                        if (strncasecmp((const char*)aggrNode->name, "aggregate", 9) == 0) 
                            break;
                    }
                }
                break;
            }
        }
    }
    if (aggrNode == NULL) 
    {
        snprintf(buf, 128, "TopologyXMLImporter::TranslateFromRspec - Cannot locate <stitching> <aggregate> element!");
        throw TEDBException(buf);
    }

    string aggrUrn = (const char*)xmlGetProp(aggrNode,  (const xmlChar*)"id");
    string domainId = GetUrnField(aggrUrn, "domain");
    Domain* aDomain = new Domain(0, domainId);

    // add AggregateReflector (AR: *:*:*) node
    sprintf(buf, "urn:publicid:IDN+%s+node+*", domainId.c_str());
    string arId = buf;
    Node* arNode = new Node(0, arId);
    aDomain->AddNode(arNode);

    // get node info: rspec/stitching/aggregate/node
    for (xmlNode = aggrNode->children; xmlNode != NULL; xmlNode = xmlNode->next)
    {
        if (xmlNode->type == XML_ELEMENT_NODE )
        {
            if (strncasecmp((const char*)xmlNode->name, "node", 4) == 0) 
            {
                // create node
                xmlChar* xmlNodeId = xmlGetProp(xmlNode,  (const xmlChar*)"id");
                string nodeId = (const char*)xmlNodeId;
                Node* aNode = new Node(0, nodeId);
                aDomain->AddNode(aNode);
                // get port info: rspec/stitching/aggregate/node/port
                xmlNodePtr xmlPortNode;
                for (xmlPortNode = xmlNode->children; xmlPortNode != NULL; xmlPortNode = xmlPortNode->next)
                {
                    if (xmlPortNode->type == XML_ELEMENT_NODE )
                    {
                        if (strncasecmp((const char*)xmlPortNode->name, "port", 4) == 0)
                        {
                            // create port
                            xmlChar* xmlPortId = xmlGetProp(xmlPortNode,  (const xmlChar*)"id");
                            string portId = (const char*)xmlPortId;
                            Port* aPort = new Port(0, portId);
                            aNode->AddPort(aPort);
                            xmlNodePtr xmlLinkNode;
                            // get link info: rspec/stitching/aggregate/node/port/link
                            for (xmlLinkNode = xmlPortNode->children; xmlLinkNode != NULL; xmlLinkNode = xmlLinkNode->next)
                            {
                                if (xmlLinkNode->type == XML_ELEMENT_NODE )
                                {
                                    if (strncasecmp((const char*)xmlLinkNode->name, "link", 4) == 0)
                                    {
                                        //$$ create link
                                        xmlChar* xmlLinkId = xmlGetProp(xmlLinkNode,  (const xmlChar*)"id");
                                        string linkId = (const char*)xmlLinkId;
                                        RLink* aRLink = new RLink(linkId);
                                        aPort->AddLink(aRLink);
                                        //$$ fill in link params
                                        xmlNodePtr xmlParamNode;
                                        for (xmlParamNode = xmlLinkNode->children; xmlParamNode != NULL; xmlParamNode = xmlParamNode->next)
                                        {
                                            if (xmlParamNode->type == XML_ELEMENT_NODE )
                                            {
                                                if (strncasecmp((const char*)xmlParamNode->name, "remoteLinkId", 10) == 0)
                                                {
                                                    xmlChar* pBuf = xmlNodeGetContent(xmlParamNode);
                                                    string rlName = (const char*)pBuf;
                                                    aRLink->SetRemoteLinkName(rlName);
                                                }
                                                else if (strncasecmp((const char*)xmlParamNode->name, "TrafficEngineeringMetric", 18) == 0)
                                                {
                                                    xmlChar* pBuf = xmlNodeGetContent(xmlParamNode);
                                                    int metric;
                                                    sscanf((const char*)pBuf, "%d", &metric);
                                                    aRLink->SetMetric(metric);
                                                }
                                                else if (strncasecmp((const char*)xmlParamNode->name, "capacity", 8) == 0)
                                                {
                                                    xmlChar* pBuf = xmlNodeGetContent(xmlParamNode);
                                                    u_int64_t bw;
                                                    sscanf((const char*)pBuf, "%llu", &bw);
                                                    aRLink->SetMaxBandwidth(bw);
                                                }
                                                else if (strncasecmp((const char*)xmlParamNode->name, "maximumReservableCapacity", 8) == 0)
                                                {
                                                    xmlChar* pBuf = xmlNodeGetContent(xmlParamNode);
                                                    u_int64_t bw;
                                                    sscanf((const char*)pBuf, "%llu", &bw);
                                                    aRLink->SetMaxReservableBandwidth(bw);
                                                }
                                                else if (strncasecmp((const char*)xmlParamNode->name, "minimumReservableCapacity", 8) == 0)
                                                {
                                                    xmlChar* pBuf = xmlNodeGetContent(xmlParamNode);
                                                    u_int64_t bw;
                                                    sscanf((const char*)pBuf, "%llu", &bw);
                                                    aRLink->SetMinReservableBandwidth(bw);
                                                }
                                                else if (strncasecmp((const char*)xmlParamNode->name, "granularity", 8) == 0)
                                                {
                                                    xmlChar* pBuf = xmlNodeGetContent(xmlParamNode);
                                                    u_int64_t bw;
                                                    sscanf((const char*)pBuf, "%llu", &bw);
                                                    aRLink->SetBandwidthGranularity(bw);
                                                }
                                                else if (strncasecmp((const char*)xmlParamNode->name, "SwitchingCapabilityDescriptors", 30) == 0)
                                                {
                                                    xmlBufferPtr buffer = xmlBufferCreate();
                                                    xmlNodeDump( buffer, rspecDoc,xmlParamNode, 0, 0);
                                                    string swcapXml = (const char*)xmlBufferContent(buffer);
                                                    aRLink->SetSwcapXmlString(swcapXml);                                                    
                                                }
                                            }
                                        }
                                        // create peering to AR (a. *:*--'*-to-nodename':* b. portname:*--"*-to-nodename-portname:*")
                                        // change remote-link-id on this link and create new port/link on AR
                                        string& remoteLinkName = aRLink->GetRemoteLinkName();
                                        size_t i1 = remoteLinkName.find("*:*:*");
                                        if (i1 != string::npos) 
                                        {
                                            string nodeShortName = GetUrnField(aNode->GetName(), "node");
                                            string portShortName = GetUrnField(aPort->GetName(), "port");
                                            sprintf(buf, "*:*-to-%s-%s:*", nodeShortName.c_str(), portShortName.c_str());
                                            remoteLinkName.replace(i1, 5, buf);
                                            aRLink->SetRemoteLinkName(remoteLinkName);
                                            string remotePortName = remoteLinkName;
                                            remotePortName.replace(remotePortName.size()-2, 2, "");
                                            Port* remotePort = new Port(0, remotePortName);
                                            remotePort->SetMaxBandwidth(aPort->GetMaxBandwidth());
                                            remotePort->SetMaxReservableBandwidth(aPort->GetMaxReservableBandwidth());
                                            remotePort->SetMinReservableBandwidth(aPort->GetMinReservableBandwidth());
                                            remotePort->SetBandwidthGranularity(aPort->GetBandwidthGranularity());
                                            arNode->AddPort(remotePort);
                                            RLink* remoteLink = new RLink(remoteLinkName);
                                            remoteLink->SetRemoteLinkName(aRLink->GetName());
                                            remoteLink->SetMetric(aRLink->GetMetric());
                                            remoteLink->SetMaxBandwidth(aPort->GetMaxBandwidth());
                                            remoteLink->SetMaxReservableBandwidth(remotePort->GetMaxReservableBandwidth());
                                            remoteLink->SetMinReservableBandwidth(remotePort->GetMinReservableBandwidth());
                                            remoteLink->SetBandwidthGranularity(remotePort->GetBandwidthGranularity());
                                            remoteLink->SetSwcapXmlString(aRLink->GetSwcapXmlString());
                                            remotePort->AddLink(remoteLink);
                                        }
                                    }
                                    else if (strncasecmp((const char*)xmlLinkNode->name, "capacity", 8) == 0)
                                    {
                                        xmlChar* pBuf = xmlNodeGetContent(xmlLinkNode);
                                        u_int64_t bw;
                                        sscanf((const char*)pBuf, "%llu", &bw);
                                        aPort->SetMaxBandwidth(bw);
                                    }
                                    else if (strncasecmp((const char*)xmlLinkNode->name, "maximumReservableCapacity", 8) == 0)
                                    {
                                        xmlChar* pBuf = xmlNodeGetContent(xmlLinkNode);
                                        u_int64_t bw;
                                        sscanf((const char*)pBuf, "%llu", &bw);
                                        aPort->SetMaxReservableBandwidth(bw);
                                    }
                                    else if (strncasecmp((const char*)xmlLinkNode->name, "minimumReservableCapacity", 8) == 0)
                                    {
                                        xmlChar* pBuf = xmlNodeGetContent(xmlLinkNode);
                                        u_int64_t bw;
                                        sscanf((const char*)pBuf, "%llu", &bw);
                                        aPort->SetMinReservableBandwidth(bw);
                                    }
                                    else if (strncasecmp((const char*)xmlLinkNode->name, "granularity", 8) == 0)
                                    {
                                        xmlChar* pBuf = xmlNodeGetContent(xmlLinkNode);
                                        u_int64_t bw;
                                        sscanf((const char*)pBuf, "%llu", &bw);
                                        aPort->SetBandwidthGranularity(bw);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    // get host info: rspec/node, rspec/node/interface, rspec/link, rspec/link/interface_ref
    // 1. get list of the non-stitching rspec links in main part
    xmlNodePtr xmlIfNode, xmlParamNode;
    list<RLink*> rspecLinks;
    for (xmlNode = rspecRoot->children; xmlNode != NULL; xmlNode = xmlNode->next)
    {
        if (xmlNode->type == XML_ELEMENT_NODE )
        {
            if (strncasecmp((const char*)xmlNode->name, "link", 4) == 0) 
            {
                list<string> ifRefs;
                u_int64_t capacity = 1000000000; //1g by default
                for (xmlIfNode = xmlNode->children; xmlIfNode != NULL; xmlIfNode = xmlIfNode->next)
                {
                    if (xmlIfNode->type == XML_ELEMENT_NODE )
                    {
                        if (strncasecmp((const char*)xmlIfNode->name, "interface_ref", 12) == 0) 
                        {                            
                            xmlChar* xmlIfId = xmlGetProp(xmlIfNode,  (const xmlChar*)"component_id");
                            string ifId = (const char*)xmlIfId;
                            ifRefs.push_back(ifId);
                        }
                        else if (strncasecmp((const char*)xmlIfNode->name, "property", 8) == 0) 
                        {                            
                            xmlChar* capStr = xmlGetProp(xmlIfNode,  (const xmlChar*)"capacity");
                            sscanf((const char*)capStr, "%llu", &capacity);
                        }
                    }
                }
                if (ifRefs.size() != 2)
                    continue;
                string nodeShortName = GetUrnField(ifRefs.front(), "node");
                string nodeId1 = "urn:publicid:IDN+";
                nodeId1 += domainId;
                nodeId1 += "+node+";
                nodeId1 += nodeShortName;
                nodeShortName = GetUrnField(ifRefs.back(), "node");
                string nodeId2 = "urn:publicid:IDN+";
                nodeId2 += domainId;
                nodeId2 += "+node+";
                nodeId2 += nodeShortName;
                bool isStitching1 = (aDomain->GetNodes().find(nodeId1) != aDomain->GetNodes().end());
                bool isStitching2 = (aDomain->GetNodes().find(nodeId2) != aDomain->GetNodes().end());
                if (isStitching1 && isStitching2) // skip a stitching only link
                    continue;
                RLink* aRLink = new RLink(ifRefs.front());
                aRLink->SetRemoteLinkName(ifRefs.back());
                aRLink->SetMaxBandwidth(capacity);
                aRLink->SetMaxReservableBandwidth(capacity);
                rspecLinks.push_back(aRLink);
            }
        }
    }
    // 2. find the host interfaces attached to stitching links that are linked to a stitching port but link (or AR) not on a stitching host 
    for (xmlNode = rspecRoot->children; xmlNode != NULL; xmlNode = xmlNode->next)
    {
        if (xmlNode->type == XML_ELEMENT_NODE )
        {
            if (strncasecmp((const char*)xmlNode->name, "node", 4) == 0) 
            {
                xmlChar* xmlNodeId = xmlGetProp(xmlNode,  (const xmlChar*)"component_id");
                string nodeId = (const char*)xmlNodeId;
                Node* aNode = NULL;
                if (aDomain->GetNodes().find(nodeId) != aDomain->GetNodes().end())
                { // handle a node in main part that is already in stithcing extension
                    aNode = (Node*)(*aDomain->GetNodes().find(nodeId)).second;
                    //add links that are not in stitching extension to node that is in stitching extension
                    for (xmlIfNode = xmlNode->children; xmlIfNode != NULL; xmlIfNode = xmlIfNode->next)
                    {
                        if (xmlIfNode->type == XML_ELEMENT_NODE )
                        {
                            if (strncasecmp((const char*)xmlIfNode->name, "interface", 9) == 0) 
                            {                            
                                xmlChar* xmlIfId = xmlGetProp(xmlIfNode,  (const xmlChar*)"component_id");
                                string ifId = (const char*)xmlIfId;
                                // get portname and linkname
                                string portName = ifId;
                                string linkName = ifId;
                                string linkShortName = GetUrnField(linkName, "link");
                                if (linkShortName.size() == 0)
                                {
                                    linkName += ":**";
                                }
                                else 
                                {
                                    portName.replace(linkName.size()-linkShortName.size()-1, linkShortName.size()+1, "");
                                }

                                bool portExisted = false, linkExisted = false;
                                map<string, Port*, strcmpless>::iterator itp = aNode->GetPorts().begin();
                                for (; itp != aNode->GetPorts().end(); itp++)
                                {
                                    Port* tp = (Port*)(*itp).second;
                                    if (tp->GetName() == portName)
                                        portExisted = true;
                                    if (tp->GetLinks().find(ifId) != tp->GetLinks().end())
                                        linkExisted = true;
                                    if (portExisted || linkExisted)
                                        break;
                                }
                                if (linkExisted)
                                    continue;
                                // otherwise, add link --
                                // 1. find the associated link in rspecLinks list
                                list<RLink*>::iterator itRL = rspecLinks.begin();
                                RLink* rspecLink = NULL;
                                for (; itRL != rspecLinks.end(); itRL++)
                                {
                                    if (linkName == (*itRL)->GetName() || linkName == (*itRL)->GetRemoteLinkName())
                                    {
                                        rspecLink = *itRL;
                                        break;
                                    }
                                }
                                if (rspecLink == NULL)
                                    continue;
                                // 2. create new port if applicable and new link
                                Port* aPort = NULL;
                                if (portExisted)
                                {
                                    aPort = (Port*)(*itp).second;
                                }
                                else
                                {
                                    aPort = new Port(0, portName);
                                    aPort->SetMaxBandwidth((*itRL)->GetMaxBandwidth());
                                    aPort->SetMaxReservableBandwidth((*itRL)->GetMaxReservableBandwidth());
                                    aPort->SetMinReservableBandwidth((*itRL)->GetMinReservableBandwidth());
                                    aPort->SetBandwidthGranularity((*itRL)->GetBandwidthGranularity());
                                    aNode->AddPort(aPort);
                                }
                                RLink* aRLink = new RLink(linkName);
                                string remoteLinkName = (ifId == rspecLink->GetName() ? rspecLink->GetRemoteLinkName() : rspecLink->GetName());
                                string remoteLinkShortName = GetUrnField(remoteLinkName, "link");
                                if (remoteLinkShortName.size() == 0)
                                {
                                    remoteLinkName += ":**";
                                }
                                aRLink->SetRemoteLinkName(remoteLinkName);
                                aRLink->SetMetric(1);
                                aRLink->SetMaxBandwidth((*itRL)->GetMaxBandwidth());
                                aRLink->SetMaxReservableBandwidth((*itRL)->GetMaxReservableBandwidth());
                                aRLink->SetMinReservableBandwidth((*itRL)->GetMinReservableBandwidth());
                                aRLink->SetBandwidthGranularity((*itRL)->GetBandwidthGranularity());
                                aRLink->SetSwcapXmlString(defaultSwcapStr);
                                aPort->AddLink(aRLink);
                            }
                        }
                    }
                }
                else 
                { // hanlde a node in main part but not in stitching extension 
                    for (xmlIfNode = xmlNode->children; xmlIfNode != NULL; xmlIfNode = xmlIfNode->next)
                    {
                        if (xmlIfNode->type == XML_ELEMENT_NODE )
                        {
                            if (strncasecmp((const char*)xmlIfNode->name, "interface", 9) == 0) 
                            {                            
                                xmlChar* xmlIfId = xmlGetProp(xmlIfNode,  (const xmlChar*)"component_id");
                                string ifId = (const char*)xmlIfId;
                                list<RLink*>::iterator itRL = rspecLinks.begin();
                                for (; itRL != rspecLinks.end(); itRL++)
                                {
                                    if (ifId == (*itRL)->GetName() || ifId == (*itRL)->GetRemoteLinkName())
                                    {
                                        aNode = new Node(0, nodeId);
                                        string portName = ifId;
                                        string linkName = ifId;
                                        string linkShortName = GetUrnField(linkName, "link");
                                        if (linkShortName.size() == 0)
                                        {
                                            linkName += ":**";
                                        }
                                        else 
                                        {
                                            portName.replace(linkName.size()-linkShortName.size()-1, linkShortName.size()+1, "");
                                        }
                                        string remoteLinkName = (ifId == (*itRL)->GetName() ? (*itRL)->GetRemoteLinkName() : (*itRL)->GetName());
                                        string remoteLinkShortName = GetUrnField(remoteLinkName, "link");
                                        if (remoteLinkShortName.size() == 0)
                                        {
                                            remoteLinkName += ":**";
                                        }
                                        map<string, Port*, strcmpless>::iterator itp = aNode->GetPorts().find(portName);
                                        Port* aPort;
                                        if (itp != aNode->GetPorts().end())
                                        {
                                            aPort = (Port*)(*itp).second;
                                        } 
                                        else
                                        {
                                            aPort = new Port(0, portName);
                                            aPort->SetMaxBandwidth((*itRL)->GetMaxBandwidth());
                                            aPort->SetMaxReservableBandwidth((*itRL)->GetMaxReservableBandwidth());
                                            aPort->SetMinReservableBandwidth((*itRL)->GetMinReservableBandwidth());
                                            aPort->SetBandwidthGranularity((*itRL)->GetBandwidthGranularity());
                                            aNode->AddPort(aPort);
                                        }
                                        RLink* aRLink = new RLink(linkName);
                                        aRLink->SetRemoteLinkName(remoteLinkName);
                                        aRLink->SetMetric(1);
                                        aRLink->SetMaxBandwidth((*itRL)->GetMaxBandwidth());
                                        aRLink->SetMaxReservableBandwidth((*itRL)->GetMaxReservableBandwidth());
                                        aRLink->SetMinReservableBandwidth((*itRL)->GetMinReservableBandwidth());
                                        aRLink->SetBandwidthGranularity((*itRL)->GetBandwidthGranularity());
                                        aRLink->SetSwcapXmlString(defaultSwcapStr);
                                        aPort->AddLink(aRLink);
                                    }
                                }
                            }
                        }
                    }
                    if (aNode != NULL)
                    {
                        aDomain->AddNode(aNode);
                    }
                }
            }
        }
    }
    char str[1024];
    sprintf(buf, "<topology xmlns=\"http://ogf.org/schema/network/topology/ctrlPlane/20110826/\" id =\"%s-t%d\"><domain id=\"%s\">",
        domainId.c_str(), (int)time(0), domainId.c_str());
    map<string, Node*, strcmpless>::iterator itn = aDomain->GetNodes().begin();
    for (; itn != aDomain->GetNodes().end(); itn++)
    {
        Node* tn = (Node*)(*itn).second;
        snprintf(str, 1024, "<node id=\"%s\">", tn->GetName().c_str());
        strcat(buf, str);
        map<string, Port*, strcmpless>::iterator itp = tn->GetPorts().begin();
        for (; itp != tn->GetPorts().end(); itp++)
        {
            Port* tp = (Port*)(*itp).second;
            snprintf(str, 1024, "<port id=\"%s\">", tp->GetName().c_str());
            strcat(buf, str);
            snprintf(str, 1024, "<capacity>%llu</capacity>", tp->GetMaxBandwidth());
            strcat(buf, str);
            snprintf(str, 1024, "<maximumReservableCapacity>%llu</maximumReservableCapacity>", tp->GetMaxReservableBandwidth());
            strcat(buf, str);
            snprintf(str, 1024, "<minimumReservableCapacity>%llu</minimumReservableCapacity>", tp->GetMinReservableBandwidth());
            strcat(buf, str);
            snprintf(str, 1024, "<granularity>%llu</granularity>", tp->GetBandwidthGranularity());
            strcat(buf, str);
            map<string, Link*, strcmpless>::iterator itl = tp->GetLinks().begin();
            for (; itl != tp->GetLinks().end(); itl++) 
            {
                RLink* tl = (RLink*)(*itl).second;
                snprintf(str, 1024, "<link id=\"%s\">", tl->GetName().c_str());
                strcat(buf, str);
                snprintf(str, 1024, "<remoteLinkId>%s</remoteLinkId>", tl->GetRemoteLinkName().c_str());
                strcat(buf, str);
                snprintf(str, 1024, "<trafficEngineeringMetric>%d</trafficEngineeringMetric>", tl->GetMetric());
                strcat(buf, str);
                snprintf(str, 1024, "<capacity>%llu</capacity>", tl->GetMaxBandwidth());
                strcat(buf, str);
                snprintf(str, 1024, "<maximumReservableCapacity>%llu</maximumReservableCapacity>", tl->GetMaxReservableBandwidth());
                strcat(buf, str);
                snprintf(str, 1024, "<minimumReservableCapacity>%llu</minimumReservableCapacity>", tl->GetMinReservableBandwidth());
                strcat(buf, str);
                snprintf(str, 1024, "<granularity>%llu</granularity>", tl->GetBandwidthGranularity());
                strcat(buf, str);
                strcat(buf, tl->GetSwcapXmlString().c_str());
                snprintf(str, 1024, "</link>");
                strcat(buf, str);
            }
            snprintf(str, 1024, "</port>");
            strcat(buf, str);
        }
        snprintf(str, 1024, "</node>");
        strcat(buf, str);
    }
    strcat(buf, "</domain></topology>");
    int sizeBuf=strlen(buf);
    xmlDocPtr xmlDoc = xmlParseMemory(buf, sizeBuf);
    return xmlDoc;
}

int GeniRequestRSpec::unique_req_id = 1;
Message* GeniRequestRSpec::CreateApiRequestMessage()
{
    if (rspecDoc == NULL)
    {
        try {
            this->ParseRspecXml();    
        } catch (exception e) {
            // TODO: Logging
            return NULL;
        }
    }
    string queueName="CORE";
    string topicName="XMLRPC_API_REQUEST";
    char tagBuf[32];
    snprintf(tagBuf, 31, "xmlrpc_api_request:%d", GeniRequestRSpec::unique_req_id);
    string contextTag= tagBuf;
    Message* msg = new Message(MSG_REQ, queueName, topicName);
    msg->SetContextTag(contextTag);
    // TODO: add request TLV
    return msg;
}

void GeniManifestRSpec::ParseApiReplyMessage(Message* msg)
{
    ; // TODO
}

