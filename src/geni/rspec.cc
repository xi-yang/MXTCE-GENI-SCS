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
#include "user_constraint.hh"
#include "compute_worker.hh"
#include "workflow.hh"

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

static string defaultSwcapStr = "<switchingCapabilityDescriptor>\
        <switchingcapType>l2sc</switchingcapType>\
        <encodingType>ethernet</encodingType>\
        <switchingCapabilitySpecificInfo>\
           <interfaceMTU>9000</interfaceMTU>\
           <vlanRangeAvailability>2-4094</vlanRangeAvailability>\
           <vlanTranslation>false</vlanTranslation>\
        </switchingCapabilitySpecificInfo>\
      </switchingCapabilityDescriptor>";

map<string, string> GeniAdRSpec::aggregateTypeMap;
map<string, string> GeniAdRSpec::aggregateUrnMap;
map<string, string> GeniAdRSpec::aggregateUrlMap;

static bool AggregateHasNestedUrn(string& domainId)
{
    string aggrType = "";
    if (GeniAdRSpec::aggregateTypeMap.find(domainId) != GeniAdRSpec::aggregateTypeMap.end())
        aggrType = GeniAdRSpec::aggregateTypeMap[domainId];
    if (aggrType.compare("orca") == 0)
        return false;
    return true;
}

xmlDocPtr GeniAdRSpec::TranslateToNML()
{
    if (rspecDoc == NULL)
        return NULL;

    char* buf = new char[1024*1024*16];
    //get domain info: rspec/stitching/aggregate
    bool isNestedUrn = true;
    xmlNodePtr rspecRoot = xmlDocGetRootElement(rspecDoc);
    xmlNodePtr xmlNode;
    xmlNodePtr aggrNode = NULL;
    string aggrType = "";
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
                        if (strncasecmp((const char*)aggrNode->name, "aggregate", 9) == 0) {
                            for (xmlNode = aggrNode->children; xmlNode != NULL; xmlNode = xmlNode->next) 
                            {
                                if (xmlNode->type == XML_ELEMENT_NODE && strncasecmp((const char*)xmlNode->name, "aggregatetype", 9) == 0) 
                                {
                                    xmlChar* pBuf = xmlNodeGetContent(xmlNode);
                                    StripXmlString(aggrType, pBuf);
                                    break;
                                }   
                            }
                            break;
                        }
                    }
                }
                break;
            }
        }
    }
    if (aggrNode == NULL) 
    {
        char errmsg[128];
        snprintf(errmsg, 128, "TopologyXMLImporter::TranslateFromRspec - Cannot locate <stitching> <aggregate> element!");
        throw TEDBException(errmsg);
    }

    string aggrUrn = (const char*)xmlGetProp(aggrNode,  (const xmlChar*)"id");
    string aggrUrl = (const char*)xmlGetProp(aggrNode,  (const xmlChar*)"url");
    string domainId = GetUrnField(aggrUrn, "domain");
    Domain* aDomain = new Domain(0, domainId);
    // create aggregate URN, URL and Type mappings
    GeniAdRSpec::aggregateTypeMap[domainId] = aggrType;
    if (aggrType.compare("orca") == 0)
    {
        isNestedUrn = false;
    }
    aDomain->setNestedUrn(isNestedUrn);
    GeniAdRSpec::aggregateUrnMap[domainId] = aggrUrn;
    vector<string> urls;
    SplitString(aggrUrl, urls, ",");
    GeniAdRSpec::aggregateUrlMap[domainId] = urls.back();
    
    // add AggregateReflector (AR: *:*:*) node/port/link
    sprintf(buf, "urn:publicid:IDN+%s+node+*", domainId.c_str());
    string arNodeId = buf;
    Node* arNode = new Node(0, arNodeId);
    aDomain->AddNode(arNode);
    sprintf(buf, "urn:publicid:IDN+%s+stitchport+*:*", domainId.c_str());
    string arPortId = buf;
    Port* arPort = new Port(0, arPortId);
    arNode->AddPort(arPort);
    arPort->SetMaxBandwidth(100000000000ULL);
    arPort->SetMaxReservableBandwidth(100000000000ULL);
    arPort->SetMinReservableBandwidth(0);
    arPort->SetBandwidthGranularity(0);
    sprintf(buf, "urn:publicid:IDN+%s+interface+*:*:*", domainId.c_str());
    string arLinkId = buf;
    RLink* arLink = new RLink(arLinkId);
    arLink->SetMetric(10);
    arLink->SetMaxBandwidth(arPort->GetMaxBandwidth());
    arLink->SetMaxReservableBandwidth(arPort->GetMaxReservableBandwidth());
    arLink->SetMinReservableBandwidth(arPort->GetMinReservableBandwidth());
    arLink->SetBandwidthGranularity(arPort->GetBandwidthGranularity());
    arLink->SetSwcapXmlString(defaultSwcapStr);
    arPort->AddLink(arLink);
    sprintf(buf, "urn:publicid:IDN+%s+interface+*:*:**", domainId.c_str());
    string arLinkId2 = buf;
    RLink* arLink2 = new RLink(arLinkId2);
    arLink2->SetMetric(10);
    arLink2->SetMaxBandwidth(arPort->GetMaxBandwidth());
    arLink2->SetMaxReservableBandwidth(arPort->GetMaxReservableBandwidth());
    arLink2->SetMinReservableBandwidth(arPort->GetMinReservableBandwidth());
    arLink2->SetBandwidthGranularity(arPort->GetBandwidthGranularity());
    arLink2->SetSwcapXmlString(defaultSwcapStr);
    arPort->AddLink(arLink2);

    // 1. import domain topology from stitching aggregate section
    // get node info: rspec/stitching/aggregate/node
    vector<string> aggrCapabilities;
    bool aggrHasAllWildcardRemoteId = false;
    for (xmlNode = aggrNode->children; xmlNode != NULL; xmlNode = xmlNode->next)
    {
        if (xmlNode->type == XML_ELEMENT_NODE )
        {
            if (strncasecmp((const char*)xmlNode->name, "capabilities", 10) == 0) 
            {
                xmlNodePtr xmlCapNode;
                for (xmlCapNode = xmlNode->children; xmlCapNode != NULL; xmlCapNode = xmlCapNode->next)
                {
                    if (xmlCapNode->type == XML_ELEMENT_NODE && (strncasecmp((const char*)xmlCapNode->name, "capability", 10) == 0))
                    {
                        xmlChar* pBuf = xmlNodeGetContent(xmlCapNode);
                        string capStr;
                        StripXmlString(capStr, pBuf);
                        aggrCapabilities.push_back(capStr);
                    }
                }
                
            }
            else if (strncasecmp((const char*)xmlNode->name, "node", 4) == 0) 
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
                        if (strncasecmp((const char*)xmlPortNode->name, "capabilities", 10) == 0) 
                        {
                            xmlNodePtr xmlCapNode;
                            for (xmlCapNode = xmlPortNode->children; xmlCapNode != NULL; xmlCapNode = xmlCapNode->next)
                            {
                                if (xmlCapNode->type == XML_ELEMENT_NODE && (strncasecmp((const char*)xmlCapNode->name, "capability", 10) == 0))
                                {
                                    xmlChar* pBuf = xmlNodeGetContent(xmlCapNode);
                                    string capStr;
                                    StripXmlString(capStr, pBuf);
				    aNode->GetCapabilities()[capStr]="true";
                                }
                            }
                        }
			else if (strncasecmp((const char*)xmlPortNode->name, "port", 4) == 0)
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
                                        string linkName = (const char*)xmlLinkId;
                                        if (aDomain->isNestedUrn())
                                        {
                                            string linkShortName = GetUrnField(linkName, "link");
                                            if (linkShortName.empty())
                                                linkName += ":**";
                                        }
                                        RLink* aRLink = new RLink(linkName);
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
                                                    string rlName;
                                                    StripXmlString(rlName, pBuf);
                                                    string domainId = GetUrnField(rlName, "domain");
                                                    // check if remote domain is nestedUrn
                                                    if (AggregateHasNestedUrn(domainId))
                                                    {
                                                        string rlShortName = GetUrnField(rlName, "link");
                                                        if (rlShortName.empty())
                                                            rlName += ":**";
                                                    }
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
                                                    string bwStr;
                                                    StripXmlString(bwStr, pBuf);
                                                    u_int64_t bw = StringToBandwidth(bwStr, 1000);
                                                    aRLink->SetMaxBandwidth(bw);
                                                }
                                                else if (strncasecmp((const char*)xmlParamNode->name, "maximumReservableCapacity", 8) == 0)
                                                {
                                                    xmlChar* pBuf = xmlNodeGetContent(xmlParamNode);
                                                    string bwStr;
                                                    StripXmlString(bwStr, pBuf);
                                                    u_int64_t bw = StringToBandwidth(bwStr, 1000);
                                                    aRLink->SetMaxReservableBandwidth(bw);
                                                }
                                                else if (strncasecmp((const char*)xmlParamNode->name, "minimumReservableCapacity", 8) == 0)
                                                {
                                                    xmlChar* pBuf = xmlNodeGetContent(xmlParamNode);
                                                    string bwStr;
                                                    StripXmlString(bwStr, pBuf);
                                                    u_int64_t bw = StringToBandwidth(bwStr, 1000);
                                                    aRLink->SetMinReservableBandwidth(bw);
                                                }
                                                else if (strncasecmp((const char*)xmlParamNode->name, "granularity", 8) == 0)
                                                {
                                                    xmlChar* pBuf = xmlNodeGetContent(xmlParamNode);
                                                    string bwStr;
                                                    StripXmlString(bwStr, pBuf);
                                                    u_int64_t bw = StringToBandwidth(bwStr, 1000);
                                                    aRLink->SetBandwidthGranularity(bw);
                                                }
                                                else if (strncasecmp((const char*)xmlParamNode->name, "switchingCapabilityDescriptor", 29) == 0)
                                                {
                                                    xmlBufferPtr buffer = xmlBufferCreate();
                                                    xmlNodeDump( buffer, rspecDoc,xmlParamNode, 0, 0);
                                                    string swcapXml = (const char*)xmlBufferContent(buffer);
                                                    aRLink->SetSwcapXmlString(swcapXml);                                                    
                                                }
                                            }
                                        }
                                        // create peering to AR (adding remotePort/Link: *:*-to-nodename-portname:*)
                                        // change remote-link-id on this link and create new port/link on AR
                                        string& remoteLinkName = aRLink->GetRemoteLinkName();
                                        size_t i1 = remoteLinkName.find("*:*:*");
                                        if (i1 != string::npos) 
                                        {
                                            aggrHasAllWildcardRemoteId = true;
                                            string nodeShortName = (aDomain->isNestedUrn() ? GetUrnField(aNode->GetName(), "node") : aNode->GetName());
                                            string portShortName = (aDomain->isNestedUrn() ? (aPort->GetName(), "port") : aPort->GetName());
                                            sprintf(buf, "*:*-to-%s-%s:*", nodeShortName.c_str(), portShortName.c_str());
                                            remoteLinkName.replace(i1, 5, buf);
                                            aRLink->SetRemoteLinkName(remoteLinkName);
                                            string remotePortName = remoteLinkName;
                                            remotePortName.replace(remotePortName.size()-2, 2, "");
                                            if (arNode->GetPorts().find(remotePortName) == arNode->GetPorts().end())
                                            {
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
                                    }
                                    else if (strncasecmp((const char*)xmlLinkNode->name, "capacity", 8) == 0)
                                    {
                                        xmlChar* pBuf = xmlNodeGetContent(xmlLinkNode);
                                        string bwStr;
                                        StripXmlString(bwStr, pBuf);
                                        u_int64_t bw = StringToBandwidth(bwStr, 1000);
                                        aPort->SetMaxBandwidth(bw);
                                    }
                                    else if (strncasecmp((const char*)xmlLinkNode->name, "maximumReservableCapacity", 8) == 0)
                                    {
                                        xmlChar* pBuf = xmlNodeGetContent(xmlLinkNode);
                                        string bwStr;
                                        StripXmlString(bwStr, pBuf);
                                        u_int64_t bw = StringToBandwidth(bwStr, 1000);
                                        aPort->SetMaxReservableBandwidth(bw);
                                    }
                                    else if (strncasecmp((const char*)xmlLinkNode->name, "minimumReservableCapacity", 8) == 0)
                                    {
                                        xmlChar* pBuf = xmlNodeGetContent(xmlLinkNode);
                                        string bwStr;
                                        StripXmlString(bwStr, pBuf);
                                        u_int64_t bw = StringToBandwidth(bwStr, 1000);
                                        aPort->SetMinReservableBandwidth(bw);
                                    }
                                    else if (strncasecmp((const char*)xmlLinkNode->name, "granularity", 8) == 0)
                                    {
                                        xmlChar* pBuf = xmlNodeGetContent(xmlLinkNode);
                                        string bwStr;
                                        StripXmlString(bwStr, pBuf);
                                        u_int64_t bw = StringToBandwidth(bwStr, 1000);
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
    // 1.2. add wildcard inbound links to all stitching nodes if nothing like that has been explicitly defined
    if (!aggrHasAllWildcardRemoteId)
    { 
        map<string, Node*, strcmpless>::iterator itn = aDomain->GetNodes().begin();
        for (; itn != aDomain->GetNodes().end(); itn++)
        {
            Node* aNode = (Node*) (*itn).second;
            string nodeShortName = (aDomain->isNestedUrn() ? GetUrnField(aNode->GetName(), "node") : aNode->GetName());
            if (nodeShortName.find("*") == 0)
                continue;
            sprintf(buf, "urn:publicid:IDN+%s+stitchport+%s:*-%s-*", domainId.c_str(), nodeShortName.c_str(), nodeShortName.c_str());
            string aPortId = buf;
            Port* aPort = new Port(0, aPortId);
            aNode->AddPort(aPort);
            aPort->SetMaxBandwidth(100000000000ULL);
            aPort->SetMaxReservableBandwidth(100000000000ULL);
            aPort->SetMinReservableBandwidth(0);
            aPort->SetBandwidthGranularity(0);
            sprintf(buf, "urn:publicid:IDN+%s+interface+%s:*-%s-*:**", domainId.c_str(), nodeShortName.c_str(), nodeShortName.c_str());
            string aLinkId = buf;
            RLink* aLink = new RLink(aLinkId);
            aLink->SetMetric(10);
            aLink->SetMaxBandwidth(aPort->GetMaxBandwidth());
            aLink->SetMaxReservableBandwidth(aPort->GetMaxReservableBandwidth());
            aLink->SetMinReservableBandwidth(aPort->GetMinReservableBandwidth());
            aLink->SetBandwidthGranularity(aPort->GetBandwidthGranularity());
            aLink->SetSwcapXmlString(defaultSwcapStr);
            aPort->AddLink(aLink);            

            sprintf(buf, "urn:publicid:IDN+%s+stitchport+*:*-to-%s-*", domainId.c_str(), nodeShortName.c_str());
            string arPortId = buf;
            Port* arPort = new Port(0, arPortId);
            arNode->AddPort(arPort);
            arPort->SetMaxBandwidth(100000000000ULL);
            arPort->SetMaxReservableBandwidth(100000000000ULL);
            arPort->SetMinReservableBandwidth(0);
            arPort->SetBandwidthGranularity(0);
            sprintf(buf, "urn:publicid:IDN+%s+interface+*:*-to-%s-*:**", domainId.c_str(), nodeShortName.c_str());
            string arLinkId = buf;
            RLink* arLink = new RLink(arLinkId);
            arLink->SetMetric(10);
            arLink->SetMaxBandwidth(arPort->GetMaxBandwidth());
            arLink->SetMaxReservableBandwidth(arPort->GetMaxReservableBandwidth());
            arLink->SetMinReservableBandwidth(arPort->GetMinReservableBandwidth());
            arLink->SetBandwidthGranularity(arPort->GetBandwidthGranularity());
            arLink->SetSwcapXmlString(defaultSwcapStr);
            arPort->AddLink(arLink);
            
            aLink->SetRemoteLinkName(arLink->GetName());
            arLink->SetRemoteLinkName(aLink->GetName());            
        }
    }

    // 2. import topology info from main rspec/node, rspec/node/interface, rspec/link, rspec/link/interface_ref
    // 2.1. get list of non-stitching rspec links in main part
    xmlNodePtr xmlIfNode, xmlParamNode;
    list<RLink*> rspecLinks;
    for (xmlNode = rspecRoot->children; xmlNode != NULL; xmlNode = xmlNode->next)
    {
        if (xmlNode->type == XML_ELEMENT_NODE )
        {
            if (strncasecmp((const char*)xmlNode->name, "link", 4) == 0) 
            {
                list<string> ifRefs;
                u_int64_t capacity = 1000000; //host interface 1g by default
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
                            string bwStr;
                            StripXmlString(bwStr, capStr);
                            capacity = StringToBandwidth(bwStr, 1000);
                        }
                    }
                }
                if (ifRefs.size() != 2)
                    continue;
                string nodeShortName = (aDomain->isNestedUrn() ? GetUrnField(ifRefs.front(), "node") : ifRefs.front());
                string nodeId1 = "urn:publicid:IDN+";
                nodeId1 += domainId;
                nodeId1 += "+node+";
                nodeId1 += nodeShortName;
                nodeShortName = (aDomain->isNestedUrn() ? GetUrnField(ifRefs.back(), "node") : ifRefs.back());
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
    // 2.2. find node/host interfaces attached to a link that points to a port already in stitching extension.
    //      Then add the related port/link if the rspec link from step 2.1 has not been added in step 1.
    //      Handle abstract port/link id (*) and urns missing link portion (append :**).
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
                { // handle a node in main part that is already in stitching extension
                    aNode = (Node*)(*aDomain->GetNodes().find(nodeId)).second;
                    // add links that connect to the node but are not in stitching extension
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
                                if (aDomain->isNestedUrn()) {
                                    string linkShortName = GetUrnField(linkName, "link");
                                    if (linkShortName.size() == 0)
                                    {
                                        linkName += ":**";
                                    }
                                    else 
                                    {
                                        portName.replace(linkName.size()-linkShortName.size()-1, linkShortName.size()+1, "");
                                    }
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
                                if (aDomain->isNestedUrn())
                                {
                                    string remoteLinkShortName = GetUrnField(remoteLinkName, "link");
                                    if (remoteLinkShortName.size() == 0)
                                    {
                                        remoteLinkName += ":**";
                                    }
                                }
                                aRLink->SetRemoteLinkName(remoteLinkName);
                                aRLink->SetMetric(10);
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
                { // handle a node in main part but not in stitching extension 
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
                                        if (aDomain->isNestedUrn())
                                        {
                                            string linkShortName = GetUrnField(linkName, "link");
                                            if (linkShortName.size() == 0)
                                            {
                                                linkName += ":**";
                                            }
                                            else 
                                            {
                                                portName.replace(linkName.size()-linkShortName.size()-1, linkShortName.size()+1, "");
                                            }
                                        }
                                        string remoteLinkName = (ifId == (*itRL)->GetName() ? (*itRL)->GetRemoteLinkName() : (*itRL)->GetName());
                                        if (aDomain->isNestedUrn())
                                        {
                                            string remoteLinkShortName = GetUrnField(remoteLinkName, "link");
                                            if (remoteLinkShortName.size() == 0)
                                            {
                                                remoteLinkName += ":**";
                                            }
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
                                        size_t i1 = remoteLinkName.rfind(":*:*");
                                        size_t i2 = remoteLinkName.rfind(":*:**");
                                        aRLink->SetMetric(10);
                                        aRLink->SetMaxBandwidth((*itRL)->GetMaxBandwidth());
                                        aRLink->SetMaxReservableBandwidth((*itRL)->GetMaxReservableBandwidth());
                                        aRLink->SetMinReservableBandwidth((*itRL)->GetMinReservableBandwidth());
                                        aRLink->SetBandwidthGranularity((*itRL)->GetBandwidthGranularity());
                                        aRLink->SetSwcapXmlString(defaultSwcapStr);
                                        if (i1 == string::npos) 
                                        {
                                            aRLink->SetRemoteLinkName(remoteLinkName);
                                        }
                                        else
                                        {
                                            // add remotePort/Link (portname:*-to-nodename-portname:*) if host is attached to abstract port (*:*)
                                            string nodeShortName = (aDomain->isNestedUrn() ? GetUrnField(aNode->GetName(), "node") : aNode->GetName());
                                            string portShortName = (aDomain->isNestedUrn() ? GetUrnField(aPort->GetName(), "port") : aPort->GetName());
                                            sprintf(buf, ":*-to-%s-%s:*", nodeShortName.c_str(), portShortName.c_str()); 
                                            if (i2 == string::npos)
                                                remoteLinkName.replace(i1, 4, buf);
                                            else
                                                remoteLinkName.replace(i1, 5, buf);
                                            aRLink->SetRemoteLinkName(remoteLinkName);
                                            string remotePortName = remoteLinkName;
                                            if (i2 == string::npos)
                                                remotePortName.replace(remotePortName.size()-2, 2, "");
                                            else
                                                remotePortName.replace(remotePortName.size()-3, 3, "");
                                            Port* remotePort = new Port(0, remotePortName);
                                            remotePort->SetMaxBandwidth(aPort->GetMaxBandwidth());
                                            remotePort->SetMaxReservableBandwidth(aPort->GetMaxReservableBandwidth());
                                            remotePort->SetMinReservableBandwidth(aPort->GetMinReservableBandwidth());
                                            remotePort->SetBandwidthGranularity(aPort->GetBandwidthGranularity());
                                            string remoteNodeShortName = (aDomain->isNestedUrn() ?GetUrnField(remoteLinkName, "node") : remoteLinkName);
                                            string remoteNodeName = "urn:publicid:IDN+";
                                            remoteNodeName += domainId;
                                            remoteNodeName += "+node+";
                                            remoteNodeName += remoteNodeShortName;
                                            Node* remoteNode = aDomain->GetNodes()[remoteNodeName];
                                            if (remoteNode != NULL)
                                            {
                                                remoteNode->AddPort(remotePort);
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
                                            else
                                            {
                                                delete remotePort;
                                            }
                                        }
                                        aPort->AddLink(aRLink);
                                    }
                                }
                            }
                        }
                    }
                    if (aNode != NULL)
                    {
                        aDomain->AddNode(aNode);
                        if (!aDomain->isNestedUrn()) // special case for ExoGENI (vmsite)
                        {
                            string trueShortDomainName = GetUrnField(aNode->GetName(), "domain");
                            string trueShortNodeName = GetUrnField(aNode->GetName(), "node");
                            if (trueShortNodeName.compare("orca-vm-cloud") != 0)
                                continue;
                            sprintf(buf, "urn:publicid:IDN+%s+stitchport+%s:*", trueShortDomainName.c_str(), trueShortNodeName.c_str());
                            string aPortId = buf;
                            Port* aPort = new Port(0, aPortId);
                            aNode->AddPort(aPort);
                            aPort->SetMaxBandwidth(100000000000ULL);
                            aPort->SetMaxReservableBandwidth(100000000000ULL);
                            aPort->SetMinReservableBandwidth(0);
                            aPort->SetBandwidthGranularity(0);
                            sprintf(buf, "urn:publicid:IDN+%s+interface+%s:*:*", trueShortDomainName.c_str(), trueShortNodeName.c_str());
                            string aLinkId = buf;
                            RLink* aLink = new RLink(aLinkId);
                            aLink->SetMetric(10);
                            aLink->SetMaxBandwidth(aPort->GetMaxBandwidth());
                            aLink->SetMaxReservableBandwidth(aPort->GetMaxReservableBandwidth());
                            aLink->SetMinReservableBandwidth(aPort->GetMinReservableBandwidth());
                            aLink->SetBandwidthGranularity(aPort->GetBandwidthGranularity());
                            aLink->SetSwcapXmlString(defaultSwcapStr);
                            aPort->AddLink(aLink);
                            if (GeniAdRSpec::aggregateUrnMap.find(trueShortDomainName) == GeniAdRSpec::aggregateUrnMap.end()) 
                            {
                                GeniAdRSpec::aggregateUrnMap[trueShortDomainName] = aggrUrn;
                                GeniAdRSpec::aggregateUrlMap[trueShortDomainName] = urls.back();
                                sprintf(buf, "urn:publicid:IDN+%s+stitchport+*:*", trueShortDomainName.c_str());
                                aPortId = buf;
                                Port* aPort = new Port(0, aPortId);
                                aNode->AddPort(aPort);
                                aPort->SetMaxBandwidth(100000000000ULL);
                                aPort->SetMaxReservableBandwidth(100000000000ULL);
                                aPort->SetMinReservableBandwidth(0);
                                aPort->SetBandwidthGranularity(0);
                                sprintf(buf, "urn:publicid:IDN+%s+interface+*:*:*", trueShortDomainName.c_str());
                                aLinkId = buf;
                                RLink* aLink = new RLink(aLinkId);
                                aLink->SetMetric(10);
                                aLink->SetMaxBandwidth(aPort->GetMaxBandwidth());
                                aLink->SetMaxReservableBandwidth(aPort->GetMaxReservableBandwidth());
                                aLink->SetMinReservableBandwidth(aPort->GetMinReservableBandwidth());
                                aLink->SetBandwidthGranularity(aPort->GetBandwidthGranularity());
                                aLink->SetSwcapXmlString(defaultSwcapStr);
                                aPort->AddLink(aLink);
                            }
                        }
                    }
                }
            }
        }
    }
    
    char str[1024];
    sprintf(buf, "<topology xmlns=\"http://ogf.org/schema/network/topology/ctrlPlane/20110826/\" id =\"%s-t%d\"><domain id=\"%s\">",
        domainId.c_str(), (int)time(0), domainId.c_str());
    if (!aDomain->isNestedUrn()) 
    {
        strcat(buf, "<isNestedUrn>false</isNestedUrn>");
    }
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
            snprintf(str, 1024, "<capacity>%lu</capacity>", tp->GetMaxBandwidth());
            strcat(buf, str);
            snprintf(str, 1024, "<maximumReservableCapacity>%lu</maximumReservableCapacity>", tp->GetMaxReservableBandwidth());
            strcat(buf, str);
            snprintf(str, 1024, "<minimumReservableCapacity>%lu</minimumReservableCapacity>", tp->GetMinReservableBandwidth());
            strcat(buf, str);
            snprintf(str, 1024, "<granularity>%lu</granularity>", tp->GetBandwidthGranularity());
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
                snprintf(str, 1024, "<capacity>%lu</capacity>", tl->GetMaxBandwidth());
                strcat(buf, str);
                snprintf(str, 1024, "<maximumReservableCapacity>%lu</maximumReservableCapacity>", tl->GetMaxReservableBandwidth());
                strcat(buf, str);
                snprintf(str, 1024, "<minimumReservableCapacity>%lu</minimumReservableCapacity>", tl->GetMinReservableBandwidth());
                strcat(buf, str);
                snprintf(str, 1024, "<granularity>%lu</granularity>", tl->GetBandwidthGranularity());
                strcat(buf, str);
                strcat(buf, tl->GetSwcapXmlString().c_str());
                snprintf(str, 1024, "</link>");
                strcat(buf, str);
            }
            snprintf(str, 1024, "</port>");
            strcat(buf, str);
        }
        if (!tn->GetCapabilities().empty())
        {
            strcat(buf, "<capabilities>");
            map<string, string>::iterator itm = tn->GetCapabilities().begin();
            for (; itm != tn->GetCapabilities().end(); itm++)
            {
                snprintf(str, 1024, "<capability>%s</capability>", ((string)itm->first).c_str());
                strcat(buf, str);
            }
            strcat(buf, "</capabilities>");
        }
        snprintf(str, 1024, "</node>");
        strcat(buf, str);
    }
    if (!aggrCapabilities.empty())
    {
        strcat(buf, "<capabilities>");
        vector<string>::iterator itCap = aggrCapabilities.begin();
        for (; itCap != aggrCapabilities.end(); itCap++)
        {
            snprintf(str, 1024, "<capability>%s</capability>", (*itCap).c_str());
            strcat(buf, str);
        }
        strcat(buf, "</capabilities>");
    }
    strcat(buf, "</domain></topology>");
    int sizeBuf=strlen(buf);
    xmlDocPtr xmlDoc = xmlParseMemory(buf, sizeBuf);
    //LOG("$$$ Topology Dump:\n"<<buf<<endl);
    delete buf;
    return xmlDoc;
}

int GeniRequestRSpec::unique_req_id = 1;

GeniRequestRSpec::~GeniRequestRSpec()
{
    map<string, Apimsg_user_constraint*>::iterator it = cachedUserConstraints.begin();
    while (it != cachedUserConstraints.end())
    {
        delete (*it).second;
        it++;
    }
}

Apimsg_user_constraint* GeniRequestRSpec::GetUserConstraintByPathId(string& id)
{
    if (cachedUserConstraints.find(id) != cachedUserConstraints.end())
        return cachedUserConstraints[id];
    return NULL;
}

Message* GeniRequestRSpec::CreateApiRequestMessage(map<string, xmlrpc_c::value>& routingProfile, bool attempt_all_paths)
{
    if (rspecDoc == NULL)
    {
        this->ParseRspecXml();    
    }

    map<string, string> rspecNs;
    rspecNs["ns"] = "http://www.geni.net/resources/rspec/3";
    rspecNs["stitch"] = "http://hpn.east.isi.edu/rspec/ext/stitch/0.1/";
    bool hasStitchingExt = (NULL != GetXpathNode(rspecDoc, "//ns:rspec//stitch:stitching//stitch:path", &rspecNs));

    string queueName="CORE";
    string topicName="XMLRPC_API_REQUEST";
    char tagBuf[32];
    snprintf(tagBuf, 31, "xmlrpc_api_request:%d", GeniRequestRSpec::unique_req_id++);
    string contextTag= tagBuf;
    Message* msg = new Message(MSG_REQ, queueName, topicName);
    msg->SetContextTag(contextTag);
    // keepers for main-body node-link information
    map<string, string> clientIdUrnMap;
    // parse request RSpec and add userConstraint TLVs
    xmlNodePtr rspecRoot = xmlDocGetRootElement(rspecDoc);
    xmlNodePtr xmlNode, pathNode, hopNode, linkNode;
    for (xmlNode = rspecRoot->children; xmlNode != NULL; xmlNode = xmlNode->next)
    {
        if (xmlNode->type == XML_ELEMENT_NODE )
        {
            if (strncasecmp((const char*)xmlNode->name, "stitching", 9) == 0) 
            {
                for (pathNode = xmlNode->children; pathNode != NULL; pathNode = pathNode->next)
                {
                    list<string>* hopInclusionList = new list<string>;
                    if (pathNode->type == XML_ELEMENT_NODE && strncasecmp((const char*)pathNode->name, "path", 4) == 0)
                    {
                        string pathId = (const char*)xmlGetProp(pathNode,  (const xmlChar*)"id");
                        xmlNodePtr linkNodeA = NULL, linkNodeZ = NULL;
                        for (hopNode = pathNode->children; hopNode != NULL; hopNode = hopNode->next)
                        {
                            if (hopNode->type == XML_ELEMENT_NODE && strncasecmp((const char*)hopNode->name, "hop", 3) == 0)
                            {
                                const char* cstrHopType = (const char*)xmlGetProp(hopNode,  (const xmlChar*)"type");
                                string hopType = "strict";
                                if (cstrHopType != NULL)
                                    hopType = cstrHopType;
                                for (linkNode = hopNode->children; linkNode != NULL; linkNode = linkNode->next)
                                {
                                    if (linkNode->type == XML_ELEMENT_NODE && strncasecmp((const char*)linkNode->name, "link", 4) == 0)
                                    {
                                        if (linkNodeA == NULL)
                                            linkNodeA = linkNode;
                                        else
                                            linkNodeZ = linkNode;
                                    }
                                    if (hopType == "strict" && linkNode->type == XML_ELEMENT_NODE
                                        && (strncasecmp((const char*)linkNode->name, "aggregate", 4) == 0
                                         || strncasecmp((const char*)linkNode->name, "node", 4) == 0
                                         || strncasecmp((const char*)linkNode->name, "port", 4) == 0
                                         || strncasecmp((const char*)linkNode->name, "link", 4) == 0) )
                                    {
                                        const char* cstrUrn = (const char*)xmlGetProp(linkNode,  (const xmlChar*)"id");
                                        if (cstrUrn != NULL)
                                        {
                                            string urn = cstrUrn;
                                            hopInclusionList->push_back(urn);
                                        }
                                    }
                                }
                            }
                        }
                        if (linkNodeA == NULL || linkNodeZ == NULL) 
                        {
                            char buf[256];
                            snprintf(buf, 256, "GeniRSpec::CreateApiRequestMessage - Failed to parse request RSpec (path id=%s)", pathId.c_str());
                            throw TEDBException(buf);
                        }
                        Apimsg_user_constraint* userCons = new Apimsg_user_constraint();
                        userCons->setGri(pathId);
                        userCons->setPathId(pathId);
                        userCons->setStarttime(time(NULL));
                        userCons->setEndtime(time(NULL)+3600*24); // scheduling attributes TBD
                        string srcLinkId = (const char*) xmlGetProp(linkNodeA, (const xmlChar*) "id");
                        userCons->setSrcendpoint(srcLinkId);
                        string dstLinkId = (const char*) xmlGetProp(linkNodeZ, (const xmlChar*) "id");
                        userCons->setDestendpoint(dstLinkId);
                        // add routing profile
                        // 1. implicit hop inclusion list from RSpec
                        if (!hopInclusionList->empty() && hopInclusionList->front() == srcLinkId)
                            hopInclusionList->pop_front();
                        if (!hopInclusionList->empty() && hopInclusionList->back() == dstLinkId)
                            hopInclusionList->pop_back();
                        // 2. explicit hop inclusion and exclusion list
                        list<string>* hopExclusionList = NULL;
                        list<string>* designatedBridgeAggrList = NULL;
                        if (routingProfile.find(pathId) != routingProfile.end())
                        {
                            map<string, xmlrpc_c::value> anRP = xmlrpc_c::value_struct(routingProfile[pathId]);
                            if (anRP.find("hop_inclusion_list") != anRP.end())
                            {
                                vector<xmlrpc_c::value> inlusionList = xmlrpc_c::value_array(anRP["hop_inclusion_list"]).vectorValueValue();
                                vector<xmlrpc_c::value>::iterator itH = inlusionList.begin();
                                for (; itH != inlusionList.end(); itH++)
                                {
                                    string hopUrn = xmlrpc_c::value_string(*itH);
                                    hopInclusionList->push_back(hopUrn);
                                }
                            }
                            if (anRP.find("hop_exclusion_list") != anRP.end())
                            {
                                vector<xmlrpc_c::value> exlusionList = xmlrpc_c::value_array(anRP["hop_exclusion_list"]).vectorValueValue();
                                vector<xmlrpc_c::value>::iterator itH = exlusionList.begin();
                                for (; itH != exlusionList.end(); itH++)
                                {
                                    string hopUrn = xmlrpc_c::value_string(*itH);
                                    if (hopExclusionList == NULL)
                                        hopExclusionList = new list<string>;
                                    hopExclusionList->push_back(hopUrn);
                                }
                            }
                            if (anRP.find("designated_bridge_aggregate_list") != anRP.end())
                            {
                                vector<xmlrpc_c::value> bridgeAggrList = xmlrpc_c::value_array(anRP["designated_bridge_aggregate_list"]).vectorValueValue();
                                vector<xmlrpc_c::value>::iterator itA = bridgeAggrList.begin();
                                for (; itA != bridgeAggrList.end(); itA++)
                                {
                                    string bridgeAggrUrn = xmlrpc_c::value_string(*itA);
                                    if (designatedBridgeAggrList == NULL)
                                        designatedBridgeAggrList = new list<string>;
                                    designatedBridgeAggrList->push_back(bridgeAggrUrn);
                                }
                            }
                        }
                        if (!hopInclusionList->empty())
                            userCons->setHopInclusionList(hopInclusionList);
                        else
                            delete hopInclusionList;
                        if (hopExclusionList != NULL)
                            userCons->setHopExclusionList(hopExclusionList);
                        if (designatedBridgeAggrList != NULL)
                            userCons->setDesignatedMPBridgeAggregates(designatedBridgeAggrList);
                        
                        xmlNodePtr xmlNode1, xmlNode2, xmlNode3, xmlNode4;
                        u_int64_t bw = 100000000; //Mbps
                        string pathType = "strict";
                        string layer = "2";
                        string srcVlan = "any";
                        string dstVlan = "any";
                        for (xmlNode1 = linkNodeA->children; xmlNode1 != NULL; xmlNode1 = xmlNode1->next) 
                        {
                            if (xmlNode1->type == XML_ELEMENT_NODE) 
                            {
                                if (strncasecmp((const char*) xmlNode1->name, "TrafficEngineeringMetric", 18) == 0) 
                                {
                                    xmlChar* pBuf = xmlNodeGetContent(xmlNode1);
                                    int metric;
                                    sscanf((const char*) pBuf, "%d", &metric);
                                    // metric not used in request
                                }
                                else if (strncasecmp((const char*) xmlNode1->name, "capacity", 8) == 0) 
                                {
                                    xmlChar* pBuf = xmlNodeGetContent(xmlNode1);
                                    string bwStr;
                                    StripXmlString(bwStr, pBuf);
                                    bw = StringToBandwidth(bwStr, 1000);
                                }
                                else if (strncasecmp((const char*) xmlNode1->name, "switchingCapabilityDescriptor", 30) == 0) 
                                {
                                    for (xmlNode2 = xmlNode1->children; xmlNode2 != NULL; xmlNode2 = xmlNode2->next) 
                                    {
                                        if (xmlNode2->type == XML_ELEMENT_NODE) 
                                        {
                                            if (strncasecmp((const char*) xmlNode2->name, "switchingcapType", 14) == 0) 
                                            {
                                                xmlChar* pBuf = xmlNodeGetContent(xmlNode2);
                                                string strCap;
                                                StripXmlString(strCap, pBuf);
                                                if (strncasecmp(strCap.c_str(), "psc", 3) == 0)
                                                    layer = "3";
                                            } 
                                            else if (strncasecmp((const char*) xmlNode2->name, "switchingCapabilitySpecificInfo", 30) == 0) 
                                            {
                                                for (xmlNode3 = xmlNode2->children; xmlNode3 != NULL; xmlNode3 = xmlNode3->next) 
                                                {
                                                    if (xmlNode3->type == XML_ELEMENT_NODE) 
                                                    {
                                                        if (strncasecmp((const char*) xmlNode3->name, "switchingCapabilitySpecificInfo_L2sc", 34) == 0) 
                                                        {
                                                            for (xmlNode4 = xmlNode3->children; xmlNode4 != NULL; xmlNode4 = xmlNode4->next) 
                                                            {
                                                                if (xmlNode4->type == XML_ELEMENT_NODE) 
                                                                {
                                                                    if (strncasecmp((const char*) xmlNode4->name, "vlanRangeAvailability", 18) == 0) 
                                                                    {
                                                                        xmlChar* pBuf = xmlNodeGetContent(xmlNode4);
                                                                        StripXmlString(srcVlan, pBuf);
                                                                    }
                                                                    else if (strncasecmp((const char*) xmlNode4->name, "suggestedVLANRange", 18) == 0) 
                                                                    {
                                                                        //;
                                                                    }
                                                                }
                                                            }

                                                        } 
                                                        else if (strncasecmp((const char*) xmlNode3->name, "switchingCapabilitySpecificInfo_Psc", 34) == 0) 
                                                        {
                                                             // TBD
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        for (xmlNode1 = linkNodeZ->children; xmlNode1 != NULL; xmlNode1 = xmlNode1->next) 
                        {
                            if (xmlNode1->type == XML_ELEMENT_NODE) 
                            {
                                if (strncasecmp((const char*) xmlNode1->name, "TrafficEngineeringMetric", 18) == 0) 
                                {
                                    xmlChar* pBuf = xmlNodeGetContent(xmlNode1);
                                    int metric;
                                    sscanf((const char*) pBuf, "%d", &metric);
                                    // metric not used in request
                                }
                                else if (strncasecmp((const char*) xmlNode1->name, "capacity", 8) == 0) 
                                {
                                    xmlChar* pBuf = xmlNodeGetContent(xmlNode1);
                                    string bwStr;
                                    StripXmlString(bwStr, pBuf);
                                    bw = StringToBandwidth(bwStr, 1000);
                                }
                                else if (strncasecmp((const char*) xmlNode1->name, "switchingCapabilityDescriptor", 30) == 0) 
                                {
                                    for (xmlNode2 = xmlNode1->children; xmlNode2 != NULL; xmlNode2 = xmlNode2->next) 
                                    {
                                        if (xmlNode2->type == XML_ELEMENT_NODE) 
                                        {
                                            if (strncasecmp((const char*) xmlNode2->name, "switchingCapabilitySpecificInfo", 30) == 0) 
                                            {
                                                for (xmlNode3 = xmlNode2->children; xmlNode3 != NULL; xmlNode3 = xmlNode3->next) 
                                                {
                                                    if (xmlNode3->type == XML_ELEMENT_NODE) 
                                                    {
                                                        if (strncasecmp((const char*) xmlNode3->name, "switchingCapabilitySpecificInfo_L2sc", 34) == 0) 
                                                        {
                                                            for (xmlNode4 = xmlNode3->children; xmlNode4 != NULL; xmlNode4 = xmlNode4->next) 
                                                            {
                                                                if (xmlNode4->type == XML_ELEMENT_NODE) 
                                                                {
                                                                    if (strncasecmp((const char*) xmlNode4->name, "vlanRangeAvailability", 18) == 0) 
                                                                    {
                                                                        xmlChar* pBuf = xmlNodeGetContent(xmlNode4);
                                                                        StripXmlString(dstVlan, pBuf);
                                                                    }
                                                                    else if (strncasecmp((const char*) xmlNode4->name, "suggestedVLANRange", 18) == 0) 
                                                                    {
                                                                        //;
                                                                    }
                                                                }
                                                            }

                                                        } 
                                                        else if (strncasecmp((const char*) xmlNode3->name, "switchingCapabilitySpecificInfo_Psc", 34) == 0) 
                                                        {
                                                             // TBD
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        userCons->setLayer(layer);
                        userCons->setPathtype(pathType);
                        userCons->setBandwidth(bw);
                        list<TSchedule*>* flexSchedules = new list<TSchedule*>;
                        TSchedule* aSchedule = new TSchedule(userCons->getStarttime(), userCons->getEndtime());
                        flexSchedules->push_back(aSchedule);
                        userCons->setFlexSchedules(flexSchedules);
                        userCons->setSrcvlantag(srcVlan);
                        userCons->setDestvlantag(dstVlan);
                        userCons->setPreserveVlanAvailabilityRange(true);
                        TLV* tlv = NULL;
                        tlv = (TLV*)(new u_int8_t[TLV_HEAD_SIZE + sizeof(userCons)]);
                        tlv->type = MSG_TLV_VOID_PTR;
                        tlv->length = sizeof(userCons);
                        memcpy(tlv->value, &userCons, sizeof(userCons));
                        Apimsg_user_constraint* copyUserCons = new Apimsg_user_constraint(*userCons);
                        // Above uses shallow copy constructor. Pointer members invalid after message sent
                        cachedUserConstraints[userCons->getPathId()] = copyUserCons;
                        msg->AddTLV(tlv);
                    }
                }
                break;
            }
            // parse node info and store client_id references
            else if (!hasStitchingExt && strncasecmp((const char*)xmlNode->name, "node", 4) == 0)
            {
                xmlChar* xmlAggrId = xmlGetProp(xmlNode, (const xmlChar*) "component_manager_id");
                string aggrName = (const char*) xmlAggrId;
                string shortAggrName = GetUrnField(aggrName, "domain");
                string shortNodeName = "*";
                xmlChar* xmlNodeId = xmlGetProp(xmlNode,  (const xmlChar*)"component_id");
                if (xmlNodeId != NULL && strstr((const char*)xmlNodeId, "urn:") != NULL)
                {
                    string nodeName = (const char*)xmlNodeId;
                    //$$$ TODO: look for domain type and determine whether isNestedUrn
                    shortNodeName = GetUrnField(nodeName, "node");
                }
                xmlNodePtr xmlIfNode;
                for (xmlIfNode = xmlNode->children; xmlIfNode != NULL; xmlIfNode = xmlIfNode->next) 
                {
                    if (xmlIfNode->type == XML_ELEMENT_NODE) 
                    {
                        if (strncasecmp((const char*) xmlIfNode->name, "interface", 9) == 0) 
                        {
                            string ifId = "";
                            xmlChar* xmlIfId = xmlGetProp(xmlIfNode, (const xmlChar*) "component_id");
                            if (xmlIfId != NULL && strstr((const char*)xmlIfId, "urn:") != NULL)
                            {
                                ifId = (const char*) xmlIfId;
                            }
                            else 
                            {
                                ifId = "urn:publicid:IDN+";
                                ifId += shortAggrName;
                                ifId += "+interface+";
                                ifId += shortNodeName;
                                ifId += ":*:*";
                            }
                            xmlChar* xmlClientId = xmlGetProp(xmlIfNode, (const xmlChar*) "client_id");
                            if (xmlClientId == NULL)
                                continue;
                            string clientId = (const char*)xmlClientId;
                            clientIdUrnMap[clientId] = ifId;
                        }
                    }
                }
            }
            // without stitching extension, use links that spans multiple aggregates
            else if (!hasStitchingExt && strncasecmp((const char*)xmlNode->name, "link", 4) == 0)
            {
                list<string> ifRefs;
                u_int64_t bw = 100000000; //100m by default
                xmlNodePtr xmlIfNode;
                for (xmlIfNode = xmlNode->children; xmlIfNode != NULL; xmlIfNode = xmlIfNode->next)
                {
                    if (xmlIfNode->type == XML_ELEMENT_NODE)
                    {
                        if (strncasecmp((const char*)xmlIfNode->name, "link_shared_vlan", 16) == 0)
                        {
                            break;
                        }
                        else if (strncasecmp((const char*)xmlIfNode->name, "link_type", 9) == 0)
                        {
                            xmlChar* pBuf =  xmlGetProp(xmlIfNode,  (const xmlChar*)"name");
                            string linkTypeValue;
                            StripXmlString(linkTypeValue, pBuf);
                            if (linkTypeValue.compare("egre-tunnel") == 0)
                                break;
                        }
                    }
                }
                // skip shared_vlan links
                if (xmlIfNode != NULL)
                    continue;
                for (xmlIfNode = xmlNode->children; xmlIfNode != NULL; xmlIfNode = xmlIfNode->next)
                {
                    if (xmlIfNode->type == XML_ELEMENT_NODE )
                    {
                        if (strncasecmp((const char*)xmlIfNode->name, "interface_ref", 12) == 0) 
                        {
                            xmlChar* xmlIfId = xmlGetProp(xmlIfNode,  (const xmlChar*)"component_id");
                            if (xmlIfId != NULL && strstr((const char*)xmlIfId, "urn:") != NULL)
                            {
                                string ifId = (const char*)xmlIfId;
                                ifRefs.push_back(ifId);
                            }
                            else 
                            {
                                // search by client_id
                                xmlChar* xmlClientId = xmlGetProp(xmlIfNode,  (const xmlChar*)"client_id");
                                if (xmlClientId == NULL)
                                    continue;
                                string clientId = (const char*)xmlClientId;
                                if (clientIdUrnMap.find(clientId) != clientIdUrnMap.end())
                                {
                                    list<string>::iterator itif = ifRefs.begin();
                                    string aggr1 = GetUrnField(clientIdUrnMap[clientId], "aggregate");
                                    for (; itif != ifRefs.end(); itif++)
                                    {
                                        string aggr2 = GetUrnField((*itif), "aggregate");
                                        if (aggr1.compare(aggr2) == 0) 
                                        {
                                            break;
                                        }
                                    }
                                    // only add aggregate-unique interface to ifRefs
                                    if (itif == ifRefs.end())
                                    {
                                        ifRefs.push_back(clientIdUrnMap[clientId]);
                                    }
                                }
                            }
                        }
                        else if (strncasecmp((const char*)xmlIfNode->name, "property", 8) == 0) 
                        {
                            xmlChar* capStr = xmlGetProp(xmlIfNode,  (const xmlChar*)"capacity");
                            if (capStr != NULL)
{
                                string bwStr;
                                StripXmlString(bwStr, capStr);
                                bw = StringToBandwidth(bwStr, 1000);
                            }
                        }
                    }
                }
                
                if (ifRefs.size() < 2)
                    continue;
                string domainA = GetUrnField(ifRefs.front(), "domain");
                string domainZ = GetUrnField(ifRefs.back(), "domain");
                if (domainA == domainZ)
                    continue;
                xmlChar* xmlLinkId = xmlGetProp(xmlNode,  (const xmlChar*)"client_id");
                if (xmlLinkId == NULL)
                    continue;
                string pathId = (const char*)xmlLinkId;
                string pathType = "strict";
                string layer = "2";
                string srcVlan = "any";//?
                string dstVlan = "any";//?
                // explicit routing_profile for multi-aggregate links
                list<string>* hopInclusionList = NULL;
                list<string>* hopExclusionList = NULL;
                list<string>* designatedBridgeAggrList = NULL;
                bool attempt_path = false;
                if (routingProfile.find(pathId) != routingProfile.end()) 
                {
                    map<string, xmlrpc_c::value> anRP = xmlrpc_c::value_struct(routingProfile[pathId]);
                    if (anRP.find("hop_inclusion_list") != anRP.end()) 
                    {
                        vector<xmlrpc_c::value> inlusionList = xmlrpc_c::value_array(anRP["hop_inclusion_list"]).vectorValueValue();
                        vector<xmlrpc_c::value>::iterator itH = inlusionList.begin();
                        for (; itH != inlusionList.end(); itH++) {
                            string hopUrn = xmlrpc_c::value_string(*itH);
                            if (hopInclusionList == NULL)
                                hopInclusionList = new list<string>;
                            hopInclusionList->push_back(hopUrn);
                        }
                    }
                    if (anRP.find("hop_exclusion_list") != anRP.end()) 
                    {
                        vector<xmlrpc_c::value> exlusionList = xmlrpc_c::value_array(anRP["hop_exclusion_list"]).vectorValueValue();
                        vector<xmlrpc_c::value>::iterator itH = exlusionList.begin();
                        for (; itH != exlusionList.end(); itH++) {
                            string hopUrn = xmlrpc_c::value_string(*itH);
                            if (hopExclusionList == NULL)
                                hopExclusionList = new list<string>;
                            hopExclusionList->push_back(hopUrn);
                        }
                    }
                    if (anRP.find("designated_bridge_aggregate_list") != anRP.end()) {
                        vector<xmlrpc_c::value> bridgeAggrList = xmlrpc_c::value_array(anRP["designated_bridge_aggregate_list"]).vectorValueValue();
                        vector<xmlrpc_c::value>::iterator itA = bridgeAggrList.begin();
                        for (; itA != bridgeAggrList.end(); itA++) {
                            string bridgeAggrUrn = xmlrpc_c::value_string(*itA);
                            if (designatedBridgeAggrList == NULL)
                                designatedBridgeAggrList = new list<string>;
                            designatedBridgeAggrList->push_back(bridgeAggrUrn);
                        }
                    }
                    if (anRP.find("attempt_path_finding") != anRP.end()) {
                        attempt_path = xmlrpc_c::value_boolean(anRP["attempt_path_finding"]);
                    }
                }
                // No stitching between two ExoGENI sites unless explict attemp_path_finding options (at top level or in routing_profile)
                if (!attempt_all_paths && !attempt_path && domainA.find("exogeni.net") != string::npos && domainZ.find("exogeni.net") != string::npos )
                    continue;
                Apimsg_user_constraint* userCons = new Apimsg_user_constraint();
                userCons->setGri(pathId);
                userCons->setPathId(pathId);
                userCons->setStarttime(time(NULL));
                userCons->setEndtime(time(NULL) + 3600 * 24); // scheduling attributes TBD
                userCons->setSrcendpoint(ifRefs.front());
                userCons->setDestendpoint(ifRefs.back());
                userCons->setLayer(layer);
                userCons->setPathtype(pathType);
                userCons->setBandwidth(bw);
                list<TSchedule*>* flexSchedules = new list<TSchedule*>;
                TSchedule* aSchedule = new TSchedule(userCons->getStarttime(), userCons->getEndtime());
                flexSchedules->push_back(aSchedule);
                userCons->setFlexSchedules(flexSchedules);
                userCons->setSrcvlantag(srcVlan);
                userCons->setDestvlantag(dstVlan);
                userCons->setPreserveVlanAvailabilityRange(true);
                if (hopInclusionList != NULL)
                    userCons->setHopInclusionList(hopInclusionList);
                if (hopExclusionList != NULL)
                    userCons->setHopExclusionList(hopExclusionList);
                if (designatedBridgeAggrList != NULL)
                    userCons->setDesignatedMPBridgeAggregates(designatedBridgeAggrList);
                // Handle multi-point bridging: !!!currently multi-point vlan bridging 
                // request is only represented through a multi-point link in rspec main body.
                if (ifRefs.size() > 2) 
                {
                    map<string, string>* mpvMap = new map<string, string>;
                    list<string>::iterator itR = ifRefs.begin();
                    for (; itR != ifRefs.end(); itR++) 
                    {
                        string vlanAny = "any";
                        (*mpvMap)[*itR] = vlanAny;
                    }
                    userCons->setMultiPointVlanMap(mpvMap);
                    topicName="XMLRPC_API_REQUEST_MPVB";
                    msg->SetTopic(topicName);
                }
                
                TLV* tlv = NULL;
                tlv = (TLV*) (new u_int8_t[TLV_HEAD_SIZE + sizeof (userCons)]);
                tlv->type = MSG_TLV_VOID_PTR;
                tlv->length = sizeof (userCons);
                memcpy(tlv->value, &userCons, sizeof (userCons));
                Apimsg_user_constraint* copyUserCons = new Apimsg_user_constraint(*userCons);
                // Above uses shallow copy constructor. Pointer members invalid after message sent
                cachedUserConstraints[userCons->getPathId()] = copyUserCons;
                msg->AddTLV(tlv);
            }
        }
    }

    if (msg->GetTLVList().empty())
    {
        char buf[256];
        snprintf(buf, 256, "GeniRSpec::CreateApiRequestMessage - no stitching path or multi-aggregate link in request RSpec.");
        throw TEDBException(buf);
        delete msg;
        return NULL;
    }
    
    return msg;
}

    
void GeniManifestRSpec::ParseApiReplyMessage(Message* msg)
{
    char buf[1024*64];
    if (msg->GetTopic() != "XMLRPC_API_REPLY")
    {
        snprintf(buf, 1024, "GeniManifestRSpec::ParseApiReplyMessage - Expecting core msg type XMLRPC_API_REPLY, not %s.", msg->GetTopic().c_str());
        throw TEDBException(buf);        
    }
    if (this->pairedRequestRspec == NULL || this->pairedRequestRspec->GetRspecXmlDoc() == NULL)
    {
        snprintf(buf, 1024, "GeniManifestRSpec::ParseApiReplyMessage - No stored Request RSpec.");
        throw TEDBException(buf);        
    }
    this->rspecDoc = xmlCopyDoc(this->pairedRequestRspec->GetRspecXmlDoc(), 1);
    xmlNodePtr rspecRoot = xmlDocGetRootElement(this->rspecDoc);
    xmlSetProp(rspecRoot,  (const xmlChar*)"type", (const xmlChar*)"request"); 
    string schemaLoc = (const char*)xmlGetProp(rspecRoot,  (const xmlChar*)"schemaLocation");
    if (schemaLoc.find("http://hpn.east.isi.edu/rspec/ext/stitch/0.1/") == string::npos) {
        schemaLoc += " http://hpn.east.isi.edu/rspec/ext/stitch/0.1/ http://hpn.east.isi.edu/rspec/ext/stitch/0.1/stitch-schema.xsd";
        if (xmlSearchNs(rspecDoc, rspecRoot,  (const xmlChar*)"xs") != NULL)
            xmlSetProp(rspecRoot,  (const xmlChar*)"xs:schemaLocation", (const xmlChar*)schemaLoc.c_str());
        else
            xmlSetProp(rspecRoot,  (const xmlChar*)"xsi:schemaLocation", (const xmlChar*)schemaLoc.c_str());
    }
    
    xmlNodePtr xmlNode, stitchingNode = NULL;
    for (xmlNode = rspecRoot->children; xmlNode != NULL; xmlNode = xmlNode->next)
    {
        if (xmlNode->type == XML_ELEMENT_NODE && strncasecmp((const char*)xmlNode->name, "stitching", 9) == 0)
        {
            stitchingNode = xmlNode;
            break;
        }
    }
    char str[1024];
    GeniTimeString(str);
    sprintf(buf, "<stitching xmlns=\"http://hpn.east.isi.edu/rspec/ext/stitch/0.1/\"  lastUpdateTime=\"%s\">", str);
    list<TLV*>::iterator it = msg->GetTLVList().begin();
    for (; it != msg->GetTLVList().end(); it++) 
    {
        TLV* tlv = (*it);
        ComputeResult* result;
        memcpy(&result, tlv->value, sizeof (void*));
        TPath* path = result->GetPathInfo();
        TGraph* graph = result->GetGraphInfo();
        string errMsg = result->GetErrMessage();
        if (errMsg.size() != 0) 
        {
            snprintf(buf, 1024, "MxTCE ComputeWorker return error message ' %s '.", errMsg.c_str());
            throw TEDBException(buf);            
        }
        else if (path == NULL && graph == NULL)
        {
            snprintf(buf, 1024, "MxTCE ComputeWorker return NULL pathInfo and NULL graphInfo for unknown error");
            throw TEDBException(buf);            
        }
        Apimsg_user_constraint* pairedUserCons = this->pairedRequestRspec->GetUserConstraintByPathId(result->GetPathId());
        if (pairedUserCons == NULL)
        {
            snprintf(buf, 1024, "GeniManifestRSpec::ParseApiReplyMessage cannot correlate path ID=%s to stored request data", result->GetPathId().c_str());
            throw TEDBException(buf);
        }
        snprintf(str, 1024, "<path id=\"%s\">", result->GetGri().c_str());
        strcat(buf, str);
        list<string> allAggregateUrns;
        char capacityCstr [16]; capacityCstr[0] = 0;
        if (graph != NULL) 
        {
            TraverseMPVBGraph(graph, buf);
            // collect allAggregateUrns and fill capacityCstr
            list<TDomain*>::iterator itD = graph->GetDomains().begin();
            for (; itD != graph->GetDomains().end(); itD++)
            {                
                TDomain *td = *itD;
                string domainId = td->GetName();
                string aggregateUrn = "";
                if (!domainId.empty() && GeniAdRSpec::aggregateUrnMap.find(domainId) != GeniAdRSpec::aggregateUrnMap.end())
                {
                    aggregateUrn = GeniAdRSpec::aggregateUrnMap[domainId];
                }
                if (!aggregateUrn.empty() && aggregateUrn.compare(allAggregateUrns.back()) != 0) 
                {
                    allAggregateUrns.push_back(aggregateUrn);
                }
            }
            list<TLink*>::iterator itL = graph->GetLinks().begin();
            for (; itL != graph->GetLinks().end(); itL++)
            {
                TLink *tl = *itL;
                string& localNodeName = tl->GetLocalEnd()->GetName();
                string& remoteNodeName = tl->GetRemoteEnd()->GetName();
                if (localNodeName.find("*") == string::npos && remoteNodeName.find("*") == string::npos)
                {
                    snprintf(capacityCstr, 16, "%lu", tl->GetMaxBandwidth()/1000);
                    break;
                }
            }
        }
        else if (path != NULL)
        {
            list<TLink*>::iterator itL = path->GetPath().begin();
            int i = 1;
            while (itL != path->GetPath().end()) 
            {
                TLink *tl = *itL;
                // trim artificial hops so i == path->GetPath().size() means last hop 
                if (tl->GetName().find("node=*") != string::npos || tl->GetName().find("port=*") != string::npos 
                        || tl->GetName().find("interface+*")!= string::npos || tl->GetName().find(":*:")!= string::npos)
                {
                    itL = path->GetPath().erase(itL);
                    continue;
                }
                string domainId = (tl->GetPort()->GetNode()->GetDomain()->isNestedUrn() ? GetUrnField(tl->GetName(), "domain") : GetUrnFieldExt(tl->GetName(), "domain"));
                string aggregateUrn = "";
                if (!domainId.empty() && GeniAdRSpec::aggregateUrnMap.find(domainId) != GeniAdRSpec::aggregateUrnMap.end())
                {
                    aggregateUrn = GeniAdRSpec::aggregateUrnMap[domainId];
                }
                if (!aggregateUrn.empty() &&
    		    (allAggregateUrns.empty() ||
		     aggregateUrn.compare(allAggregateUrns.back()) != 0))
                {
                    allAggregateUrns.push_back(aggregateUrn);
                }
                list<TLink*>::iterator itL2 = itL;
                itL2++;
                while (itL2 != path->GetPath().end() && 
                    ((*itL2)->GetName().find("node=*") != string::npos || (*itL2)->GetName().find("port=*") != string::npos
                        ||  (*itL2)->GetName().find("-*:") != string::npos ||  (*itL2)->GetName().find(":*-") != string::npos))
                {
                    itL2 = path->GetPath().erase(itL2);
                }
                // rearrange available VLAN tags for GENI workflow
                string newVlanRange = "any";
                if (i == 1) // re-set first hop to use srcVlanRange
                {
                    newVlanRange = pairedUserCons->getSrcvlantag();
                }
                else if (i == path->GetPath().size())
                {
                    newVlanRange = pairedUserCons->getDestvlantag();

                }
                // else set middle hops to use any
                
                snprintf(str, 1024, "<hop id=\"%d\">", i);
                strcat(buf, str);
                string& linkName = tl->GetName();
                size_t iErase = linkName.find(":link=**");
                if (iErase != string::npos)
                {
                    linkName.erase(linkName.begin()+iErase, linkName.end());
                } 
                else 
                {
                    iErase = linkName.find(":**");
                    if (iErase != string::npos)
                    {
                        linkName.erase(linkName.begin()+iErase, linkName.end());
                    } 
                }
                snprintf(str, 1024, "<link id=\"%s\">", (tl->GetPort()->GetNode()->GetDomain()->isNestedUrn() ? ConvertLinkUrn_Dnc2Geni(linkName).c_str() : ConvertLinkUrn_Dnc2GeniExt(linkName).c_str()));
                strcat(buf, str);
                snprintf(str, 1024, "<trafficEngineeringMetric>%d</trafficEngineeringMetric>", tl->GetMetric());
                strcat(buf, str);
                if (capacityCstr[0] == 0) 
                {
                    snprintf(capacityCstr, 16, "%lu", tl->GetMaxBandwidth()/1000);
                }
                list<ISCD*>::iterator its = tl->GetSwCapDescriptors().begin();
                for (; its != tl->GetSwCapDescriptors().end(); its++) 
                {
                    ISCD *iscd = *its;
                    snprintf(str, 1024, "<capacity>%lu</capacity>", tl->GetMaxBandwidth()/1000);
                    strcat(buf, str);
                    snprintf(str, 1024, "<switchingCapabilityDescriptor>");
                    strcat(buf, str);
                    if (iscd->switchingType == LINK_IFSWCAP_L2SC)
                        snprintf(str, 1024, "<switchingcapType>l2sc</switchingcapType>");
                    else if (iscd->switchingType == LINK_IFSWCAP_TDM)
                        snprintf(str, 1024, "<switchingcapType>tdm</switchingcapType>");
                    else if (iscd->switchingType == LINK_IFSWCAP_LSC)
                        snprintf(str, 1024, "<switchingcapType>lsc</switchingcapType>");
                    else if (iscd->switchingType >= LINK_IFSWCAP_PSC1 && iscd->switchingType <= LINK_IFSWCAP_PSC4)
                        snprintf(str, 1024, "<switchingcapType>psc</switchingcapType>");
                    strcat(buf, str);
                    if (iscd->switchingType == LINK_IFSWCAP_ENC_ETH)
                        snprintf(str, 1024, "<encodingType>ethernet</encodingType>");
                    else if (iscd->switchingType == LINK_IFSWCAP_ENC_PKT)
                        snprintf(str, 1024, "<encodingType>packet</encodingType>");
                    else if (iscd->switchingType == LINK_IFSWCAP_ENC_LAMBDA)
                        snprintf(str, 1024, "<encodingType>lambda</encodingType>");
                    else //default = ethernet
                        snprintf(str, 1024, "<encodingType>ethernet</encodingType>");
                    strcat(buf, str);
                    snprintf(str, 1024, "<switchingCapabilitySpecificInfo>");
                    strcat(buf, str);
                    if (newVlanRange.compare("any") == 0)
                    {
                        newVlanRange = ((ISCD_L2SC*)iscd)->availableVlanTags.GetRangeString();
                    }
                    if (iscd->switchingType == LINK_IFSWCAP_L2SC)
                    {
                        snprintf(str, 1024, "<switchingCapabilitySpecificInfo_L2sc>");
                        strcat(buf, str);
                        if (((ISCD_L2SC*)iscd)->mtu > 0) 
                        {
                            snprintf(str, 1024, "<interfaceMTU>%d</interfaceMTU>", ((ISCD_L2SC*)iscd)->mtu);
                            strcat(buf, str);
                        }
                        snprintf(str, 1024, "<vlanRangeAvailability>%s</vlanRangeAvailability>", newVlanRange.c_str());
                        strcat(buf, str);
                        snprintf(str, 1024, "<suggestedVLANRange>%s</suggestedVLANRange>", ((ISCD_L2SC*)iscd)->suggestedVlanTags.GetRangeString().c_str());
                        strcat(buf, str);
                        snprintf(str, 1024, "<vlanTranslation>%s</vlanTranslation>", ((ISCD_L2SC*)iscd)->vlanTranslation ? "true":"false");
                        strcat(buf, str);
                        snprintf(str, 1024, "</switchingCapabilitySpecificInfo_L2sc>");
                        strcat(buf, str);
                    }
                    snprintf(str, 1024, "</switchingCapabilitySpecificInfo>");
                    strcat(buf, str);
                    if (iscd->VendorSpecificInfo() != NULL && !iscd->VendorSpecificInfo()->GetXmlByString().empty())
                        strcat(buf, iscd->VendorSpecificInfo()->GetXmlByString().c_str());
                    snprintf(str, 1024, "</switchingCapabilityDescriptor>");
                    strcat(buf, str);
                }
                if (!tl->GetCapabilities().empty())
                {
                    snprintf(str, 1024, "<capabilities>");
                    strcat(buf, str);
                    map<string, string, strcmpless>::iterator itc = tl->GetCapabilities().begin();
                    for (; itc != tl->GetCapabilities().end(); itc++)
                    {
                        snprintf(str, 1024, "<capability>%s</capability>", ((*itc).first).c_str());
                        strcat(buf, str);
                    }
                    snprintf(str, 1024, "</capabilities>");
                    strcat(buf, str);
                }
                snprintf(str, 1024, "</link>");
                strcat(buf, str);
                list<TLink*>::iterator itNext = itL;
                itNext++;
                while (itNext != path->GetPath().end()) 
                {
                    TLink* tlx = *itNext;
                    if (tlx->GetName().find("node=*") != string::npos || tlx->GetName().find("port=*") != string::npos 
                      || tlx->GetName().find("interface+*")!= string::npos || tlx->GetName().find(":*:")!= string::npos)
                        itNext++;
                    else
                        break;
                }
                if (itNext == path->GetPath().end())
                    snprintf(str, 1024, "<nextHop>null</nextHop>");
                else
                    snprintf(str, 1024, "<nextHop>%d</nextHop>", i + 1);
                strcat(buf, str);
                snprintf(str, 1024, "</hop>");
                strcat(buf, str);
                i++;
                itL++;
            }
        }
        snprintf(str, 1024, "</path>");
        strcat(buf, str);
        
        // collected allAggregateUrns from above loop -> use these to add component_manager(s) to main body link
        if (allAggregateUrns.size() >= 2)
        {
            string srcAggrUrn = allAggregateUrns.front();
            string dstAggrUrn = allAggregateUrns.back();
            
            // look for links with client_id == pathId 
            // insert extra <component_manager> elements                 
            map<string, string> rspecNs;
            rspecNs["ns"] = "http://www.geni.net/resources/rspec/3";
            rspecNs["stitch"] = "http://hpn.east.isi.edu/rspec/ext/stitch/0.1/";
            sprintf(str, "//ns:rspec/ns:link[@client_id='%s']", result->GetGri().c_str());
            xmlNodePtr linkXmlNode = GetXpathNode(this->rspecDoc, str, &rspecNs);
            if (linkXmlNode != NULL) 
            {
                if (allAggregateUrns.size() > 2)
                {
                    sprintf(str, "//ns:rspec/ns:link[@client_id='%s']/ns:component_manager[contains(@name,'%s')]", 
                            result->GetGri().c_str(), srcAggrUrn.c_str());
                    xmlNodePtr srcAggrXmlNode = GetXpathNode(this->rspecDoc, str, &rspecNs);
                    if (srcAggrXmlNode != NULL)
                    {
                        allAggregateUrns.pop_front();                
                    }
                    sprintf(str, "//ns:rspec/ns:link[@client_id='%s']/ns:component_manager[contains(@name,'%s')]", 
                            result->GetGri().c_str(), dstAggrUrn.c_str());
                    xmlNodePtr dstAggrXmlNode = GetXpathNode(this->rspecDoc, str, &rspecNs);
                    if (dstAggrXmlNode == NULL)
                    {
                        dstAggrXmlNode = xmlNewNode(NULL, BAD_CAST "component_manager");
                        xmlNewProp(dstAggrXmlNode, BAD_CAST "name", BAD_CAST dstAggrUrn.c_str());
                        xmlAddChild(linkXmlNode, dstAggrXmlNode);
                    }
                    allAggregateUrns.pop_back();
                    list<string>::iterator itUrn = allAggregateUrns.begin();
                    for (; itUrn != allAggregateUrns.end(); itUrn++)
                    {
                        xmlNodePtr aggrXmlNode = xmlNewNode(NULL, BAD_CAST "component_manager");
                        xmlNewProp(aggrXmlNode, BAD_CAST "name", BAD_CAST (*itUrn).c_str());
                        xmlAddPrevSibling(dstAggrXmlNode, aggrXmlNode);
                    }
                }
                sprintf(str, "//ns:rspec/ns:link[@client_id='%s']/ns:interface_ref", 
                        result->GetGri().c_str(), srcAggrUrn.c_str());
                xmlNodeSetPtr interfaceXmlNodeSet = GetXpathNodeSet(this->rspecDoc, str, &rspecNs);
                if (interfaceXmlNodeSet->nodeNr == 2) {
                    xmlNodePtr srcIfNode = interfaceXmlNodeSet->nodeTab[0];
                    xmlChar* srcIfId = xmlGetProp(srcIfNode,  BAD_CAST "client_id");
                    xmlNodePtr dstIfNode = interfaceXmlNodeSet->nodeTab[1];
                    xmlChar* dstIfId = xmlGetProp(dstIfNode,  BAD_CAST "client_id");
                    sprintf(str, "//ns:rspec/ns:link[@client_id='%s']/ns:property[contains(@source_id,'%s')]", 
                            result->GetGri().c_str(), srcIfId);
                    xmlNodePtr propertyXmlNode = GetXpathNode(this->rspecDoc, str, &rspecNs); 
                    if (propertyXmlNode == NULL) {
                        propertyXmlNode = xmlNewNode(NULL, BAD_CAST "property");
                        xmlNewProp(propertyXmlNode, BAD_CAST "capacity", BAD_CAST capacityCstr);
                        xmlNewProp(propertyXmlNode, BAD_CAST "source_id", srcIfId);
                        xmlNewProp(propertyXmlNode, BAD_CAST "dest_id", dstIfId);
                        xmlNewProp(propertyXmlNode, BAD_CAST "latency", BAD_CAST "0");
                        xmlNewProp(propertyXmlNode, BAD_CAST "packet_loss", BAD_CAST "0");
                        xmlAddChild(linkXmlNode, propertyXmlNode);                        
                    }
                    sprintf(str, "//ns:rspec/ns:link[@client_id='%s']/ns:property[contains(@dest_id,'%s')]", 
                            result->GetGri().c_str(), srcIfId);
                    propertyXmlNode = GetXpathNode(this->rspecDoc, str, &rspecNs); 
                    if (propertyXmlNode == NULL) {
                        propertyXmlNode = xmlNewNode(NULL, BAD_CAST "property");
                        xmlNewProp(propertyXmlNode, BAD_CAST "capacity", BAD_CAST capacityCstr);
                        xmlNewProp(propertyXmlNode, BAD_CAST "source_id", dstIfId);
                        xmlNewProp(propertyXmlNode, BAD_CAST "dest_id", srcIfId);
                        xmlNewProp(propertyXmlNode, BAD_CAST "latency", BAD_CAST "0");
                        xmlNewProp(propertyXmlNode, BAD_CAST "packet_loss", BAD_CAST "0");
                        xmlAddChild(linkXmlNode, propertyXmlNode);                        
                    }
                }
            }
        }
        // extract workflow data
        WorkflowData *workflowData = new WorkflowData();
        if (graph != NULL)
        {
            vector<TPath*> treePaths = graph->TreeToPaths(graph->GetNodes().front());
            if (treePaths.empty())
            {
                snprintf(buf, 1024, "GeniManifestRSpec::ParseApiReplyMessage MPVB TGraph::TreeToPaths returns none.");
                throw TEDBException(buf);
            }
            vector<TPath*>::iterator itP = treePaths.begin();
            workflowData->LoadPath(*itP);
            workflowData->ComputeDependency();
            for (++itP; itP != treePaths.end(); itP++)
            {
                TPath* tp = *itP;
                WorkflowData workflowAdd;
                workflowAdd.LoadPath(tp);
                workflowAdd.ComputeDependency();
                workflowData->MergeDependencies(workflowAdd.GetDependencies());
            }
        }
        else if (path != NULL) 
        {
            workflowData->LoadPath(path);
            workflowData->ComputeDependency();
        }
        workflowData->GenerateXmlRpcData();
        this->workflowDataMap[result->GetPathId()] = workflowData;
    }
    sprintf(str, "</stitching>");
    strcat(buf, str);
    int sizeBuf=strlen(buf);
    xmlDocPtr newStitchingDoc = xmlParseMemory(buf, sizeBuf);
    if (newStitchingDoc == NULL)
    {
        snprintf(buf, 1024, "GeniManifestRSpec::ParseApiReplyMessage failed to compose XML for manifest paths.");
        throw TEDBException(buf);                    
    }
    xmlNodePtr newStitchingNode = xmlDocGetRootElement(newStitchingDoc);
    if (stitchingNode)
        xmlReplaceNode(stitchingNode, newStitchingNode);
    else
        xmlAddChild(rspecRoot, newStitchingNode);
    
    
    this->DumpRspecXml();
}

void GeniManifestRSpec::TraverseMPVBGraph(TGraph* graph, char* buf)
{
    list<TNode*>::iterator itN = graph->GetNodes().begin();
    for (; itN != graph->GetNodes().end(); itN++)
    {
        TNode* node = *itN;
        bool*  visited = (bool*)node->GetWorkData()->GetData("MPVB_VISITED");
        *visited = false;
    }    
    list<TLink*>::iterator itL = graph->GetLinks().begin();
    for (; itL != graph->GetLinks().end(); itL++)
    {
        Link* link = *itL;
        bool*  visited = (bool*)link->GetWorkData()->GetData("MPVB_VISITED");
        *visited = false;
        link->GetWorkData()->SetData("NEXT_HOPS", new list<TLink*>);
        link->GetWorkData()->SetData("HOP_ID", new int(0));
    }
    // create nextHop relationships for links that matter (skip artificial ones)
    int hopCount = 0;
    MPVB_Nexthop_Recursive(graph->GetNodes().front(), NULL, &hopCount);
    // find the firstHop to start recursive finalize procedure
    TLink* firstHopLink = NULL;
    for (itL = graph->GetLinks().begin(); itL != graph->GetLinks().end(); itL++)
    {
        TLink* link = *itL;
        int hopId = *(int*)link->GetWorkData()->GetData("HOP_ID");
        if (hopId == 1)
            firstHopLink = link;
    }
    // follow the nextHop relationships to traverse the tree and create path hops XML
    MPVB_Finalize_Recursive(firstHopLink, buf);
}

void GeniManifestRSpec::MPVB_Nexthop_Recursive(TNode* node, TLink* previousLink, int* hopCount)
{
    list<TLink*>::iterator itL = node->GetLocalLinks().begin();
    for (itL = node->GetLocalLinks().begin(); itL != node->GetLocalLinks().end(); itL++)
    {
        TLink* localLink = *itL;
        if (localLink == previousLink)
            continue;
        bool* visited = (bool*)localLink->GetWorkData()->GetData("MPVB_VISITED");
        if (*visited)
            continue;
        *visited = true;
        TLink* remoteLink = (TLink*)localLink->GetRemoteLink();
        if (remoteLink == NULL)
            continue;
        visited = (bool*)remoteLink->GetWorkData()->GetData("MPVB_VISITED");
        if (*visited)
            continue;
        *visited = true;
        TNode* nextNode = remoteLink->GetLocalEnd();
        if (nextNode == NULL)
            continue;
        string& nodeName = node->GetName();
        string& nextNodeName = nextNode->GetName();
        if (nodeName.find("*") != string::npos || nextNodeName.find("*") != string::npos)
        {
            // carry on to skip both localLink and remoteLink and pass previousLink to nextNode
            MPVB_Nexthop_Recursive(nextNode, previousLink, hopCount);
            continue;
        }
        // add localLink as nextHop of previousLink
        if (previousLink != NULL)
        {
            list<TLink*>& previousLinkNextHops = *(list<TLink*>*)previousLink->GetWorkData()->GetData("NEXT_HOPS");
            previousLinkNextHops.push_back(localLink);
        }
        // add remoteLink as nextHop of localLink
        list<TLink*>& localLinkNextHops = *(list<TLink*>*)localLink->GetWorkData()->GetData("NEXT_HOPS");
        localLinkNextHops.push_back(remoteLink);
        int* localLinkHopId = (int*)localLink->GetWorkData()->GetData("HOP_ID");
        if (*localLinkHopId == 0)
            *localLinkHopId = ++(*hopCount);
        int* remoteLinkHopId = (int*)remoteLink->GetWorkData()->GetData("HOP_ID");
        if (*remoteLinkHopId == 0)
            *remoteLinkHopId = ++(*hopCount);
        // carry on recursion with preivousLink = remoteLink
        MPVB_Nexthop_Recursive(nextNode, remoteLink, hopCount);
    }
}

void GeniManifestRSpec::MPVB_Finalize_Recursive(TLink* previousLink, char* buf)
{
    char str[1024];
    char capacityCstr[16];

    // create hop XML for previousLink
    int* previousLinkHopId = (int*) previousLink->GetWorkData()->GetData("HOP_ID");
    u_int32_t previousLinkSuggestedVlan = *(u_int32_t*) previousLink->GetWorkData()->GetData("SUGGESTED_VLAN");
    snprintf(str, 1024, "<hop id=\"%d\">", *previousLinkHopId);
    strcat(buf, str);
    string& linkName = previousLink->GetName();
    if (strstr(linkName.c_str(), "urn:ogf:network") == NULL) 
    {
        sprintf(str, "urn:ogf:network:domain=%s:node=%s:port=%s:link=%s",
                previousLink->GetPort()->GetNode()->GetDomain()->GetName().c_str(),
                previousLink->GetPort()->GetNode()->GetName().c_str(),
                previousLink->GetPort()->GetName().c_str(),
                previousLink->GetName().c_str());
        linkName = str;
    }
    size_t iErase = linkName.find(":link=**");
    if (iErase != string::npos) 
    {
        linkName.erase(linkName.begin() + iErase, linkName.end());
    }
    else 
    {
        iErase = linkName.find(":**");
        if (iErase != string::npos) {
            linkName.erase(linkName.begin() + iErase, linkName.end());
        }
    }
    snprintf(str, 1024, "<link id=\"%s\">", (previousLink->GetPort()->GetNode()->GetDomain()->isNestedUrn() ? ConvertLinkUrn_Dnc2Geni(linkName).c_str() : ConvertLinkUrn_Dnc2GeniExt(linkName).c_str()));
    strcat(buf, str);
    snprintf(str, 1024, "<trafficEngineeringMetric>%d</trafficEngineeringMetric>", previousLink->GetMetric());
    strcat(buf, str);
    snprintf(capacityCstr, 16, "%lu", previousLink->GetMaxBandwidth() / 1000);
    list<ISCD*>::iterator its = previousLink->GetSwCapDescriptors().begin();
    for (; its != previousLink->GetSwCapDescriptors().end(); its++) 
    {
        ISCD *iscd = *its;
        snprintf(str, 1024, "<capacity>%lu</capacity>", previousLink->GetMaxBandwidth() / 1000);
        strcat(buf, str);
        snprintf(str, 1024, "<switchingCapabilityDescriptor>");
        strcat(buf, str);
        if (iscd->switchingType == LINK_IFSWCAP_L2SC)
            snprintf(str, 1024, "<switchingcapType>l2sc</switchingcapType>");
        else if (iscd->switchingType == LINK_IFSWCAP_TDM)
            snprintf(str, 1024, "<switchingcapType>tdm</switchingcapType>");
        else if (iscd->switchingType == LINK_IFSWCAP_LSC)
            snprintf(str, 1024, "<switchingcapType>lsc</switchingcapType>");
        else if (iscd->switchingType >= LINK_IFSWCAP_PSC1 && iscd->switchingType <= LINK_IFSWCAP_PSC4)
            snprintf(str, 1024, "<switchingcapType>psc</switchingcapType>");
        strcat(buf, str);
        if (iscd->switchingType == LINK_IFSWCAP_ENC_ETH)
            snprintf(str, 1024, "<encodingType>ethernet</encodingType>");
        else if (iscd->switchingType == LINK_IFSWCAP_ENC_PKT)
            snprintf(str, 1024, "<encodingType>packet</encodingType>");
        else if (iscd->switchingType == LINK_IFSWCAP_ENC_LAMBDA)
            snprintf(str, 1024, "<encodingType>lambda</encodingType>");
        else //default = ethernet
            snprintf(str, 1024, "<encodingType>ethernet</encodingType>");
        strcat(buf, str);
        snprintf(str, 1024, "<switchingCapabilitySpecificInfo>");
        strcat(buf, str);
        string newVlanRange = ((ISCD_L2SC*) iscd)->availableVlanTags.GetRangeString();
        if (iscd->switchingType == LINK_IFSWCAP_L2SC) 
        {
            snprintf(str, 1024, "<switchingCapabilitySpecificInfo_L2sc>");
            strcat(buf, str);
            if (((ISCD_L2SC*) iscd)->mtu > 0) 
            {
                snprintf(str, 1024, "<interfaceMTU>%d</interfaceMTU>", ((ISCD_L2SC*) iscd)->mtu);
                strcat(buf, str);
            }
            snprintf(str, 1024, "<vlanRangeAvailability>%s</vlanRangeAvailability>", newVlanRange.c_str());
            strcat(buf, str);
            snprintf(str, 1024, "<suggestedVLANRange>%d</suggestedVLANRange>", previousLinkSuggestedVlan);
            strcat(buf, str);
            snprintf(str, 1024, "<vlanTranslation>%s</vlanTranslation>", ((ISCD_L2SC*) iscd)->vlanTranslation ? "true" : "false");
            strcat(buf, str);
            snprintf(str, 1024, "</switchingCapabilitySpecificInfo_L2sc>");
            strcat(buf, str);
        }
        snprintf(str, 1024, "</switchingCapabilitySpecificInfo>");
        strcat(buf, str);
        if (iscd->VendorSpecificInfo() != NULL && !iscd->VendorSpecificInfo()->GetXmlByString().empty())
            strcat(buf, iscd->VendorSpecificInfo()->GetXmlByString().c_str());
        snprintf(str, 1024, "</switchingCapabilityDescriptor>");
        strcat(buf, str);
    }
    if (!previousLink->GetCapabilities().empty()) 
    {
        snprintf(str, 1024, "<capabilities>");
        strcat(buf, str);
        map<string, string, strcmpless>::iterator itc = previousLink->GetCapabilities().begin();
        for (; itc != previousLink->GetCapabilities().end(); itc++) 
        {
            snprintf(str, 1024, "<capability>%s</capability>", ((*itc).first).c_str());
            strcat(buf, str);
        }
        snprintf(str, 1024, "</capabilities>");
        strcat(buf, str);
    }
    snprintf(str, 1024, "</link>");
    strcat(buf, str);
    // add nextHops
    list<TLink*>::iterator itL;
    list<TLink*>& nextHopLinks = *(list<TLink*>*)previousLink->GetWorkData()->GetData("NEXT_HOPS");
    bool isLeaf = true;
    for (itL = nextHopLinks.begin(); itL != nextHopLinks.end(); itL++) 
    {
        TLink* nextLink = *itL;
        isLeaf = false;
        int* localLinkHopId = (int*) nextLink->GetWorkData()->GetData("HOP_ID");
        // add loalLink as nextHop to previousLink
        if (previousLink != NULL) 
        {
            snprintf(str, 1024, "<nextHop>%d</nextHop>", *localLinkHopId);
            strcat(buf, str);
        }
    }
    if (isLeaf) 
    {
        snprintf(str, 1024, "<nextHop>null</nextHop>");
        strcat(buf, str);
    }
    snprintf(str, 1024, "</hop>");
    strcat(buf, str);
    // preceed recursively to nextHop Links
    for (itL = nextHopLinks.begin(); itL != nextHopLinks.end(); itL++) 
    {
        TLink* nextLink = *itL;
        MPVB_Finalize_Recursive(nextLink, buf);
    }
}


