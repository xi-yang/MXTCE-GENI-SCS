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

#include "topology_importer.hh"


void TopologyXMLImporter::Init()
{
    list<string>::iterator itF = xmlFilePathList.begin();
    for (; itF != xmlFilePathList.end(); itF++)
    {
        fileModTimeList.push_back((time_t)0);
    }
}

void TopologyXMLImporter::Run()
{
    char buf[128];
    list<string>::iterator itF = xmlFilePathList.begin();
    list<time_t>::iterator itT = fileModTimeList.begin();
    bool hasUpdate = false;
    for (; itF != xmlFilePathList.end(); itF++, itT++)
    {
        time_t lastModTime = get_mtime((const char *)(*itF).c_str());
        if (lastModTime == 0)
        {
            snprintf(buf, 128, "TopologyXMLImporter::Run failed to open XML topology file: %s", (*itF).c_str());
            throw TEDBException(buf);
        }
        if ((*itT) >= lastModTime)  // XML topology file has not been modified since last import. 
            continue;
        (*itT) = lastModTime;
        hasUpdate = true;
        xmlDocPtr xmlDoc = xmlParseFile((*itF).c_str());
        if (xmlDoc == NULL)
        {
            snprintf(buf, 128, "TopologyXMLImporter::Run - Failed to parse XML topology file: %s", (*itF).c_str());
            throw TEDBException(buf);
        }
        tedb->LockDB();
        string fileType = this->CheckFileType(xmlDoc);
        if (fileType == "unknown") 
        {
            snprintf(buf, 128, "TopologyXMLImporter::Run - Unknown type of XML topology file: %s", (*itF).c_str());
            throw TEDBException(buf);
        }
        else if (fileType == "rspec") 
        {
            xmlDocPtr xmlDocOrig = xmlDoc;
            xmlDoc = this->TranslateFromRspec(xmlDocOrig);
            xmlFreeDoc(xmlDocOrig);
        }
        tedb->AddXmlDomainTree(xmlDoc);
        tedb->UnlockDB();
    }
    if (hasUpdate)
    {
        tedb->LockDB();
        tedb->ClearXmlTrees();
        tedb->PopulateXmlTrees();
        tedb->UnlockDB();
    }
};

string TopologyXMLImporter::CheckFileType(xmlDocPtr xmlDoc)
{
    xmlNodePtr xmlRoot = xmlDocGetRootElement(xmlDoc);
    if (xmlRoot->type == XML_ELEMENT_NODE && strncasecmp((const char*)xmlRoot->name, "topology", 8) == 0)
    {
        return "nml";
    }
    else if (xmlRoot->type == XML_ELEMENT_NODE && strncasecmp((const char*)xmlRoot->name, "rspec", 8) == 0)
    {
        return "rspec";
    }
    return "unknown";
}

xmlDocPtr TopologyXMLImporter::TranslateFromRspec(xmlDocPtr rspecDoc)
{
    char buf[1024];
    //strcpy(buf, "<?xml version=\"1.0\"?><topology xmlns=\"http://ogf.org/schema/network/topology/ctrlPlane/20110826/\" id=\"\" />");
    strcpy(buf, "<topology />");
    int sizeBuf=strlen(buf);
    xmlDocPtr xmlDoc = xmlParseMemory(buf, sizeBuf);

    //$$ get domain info: rspec/stitching/aggregate
        //$$ create domain
        //$$ fill in topology id and domain id
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

    xmlChar* xmlAggrId = xmlGetProp(aggrNode,  (const xmlChar*)"id");
    xmlNodePtr xmlRoot = xmlDocGetRootElement(xmlDoc);
    string aggrUrn = (const char*)xmlAggrId;
    string domainId = GetUrnField(aggrUrn, "domain");
    xmlSetProp(xmlRoot, (const xmlChar*)"xmlns", (const xmlChar*)"http://ogf.org/schema/network/topology/ctrlPlane/20110826/");
    xmlSetProp(xmlRoot, (const xmlChar*)"id", (const xmlChar*)domainId.c_str());
    Domain* aDomain = new Domain(0, domainId);
    
    //$$ add AggregateReflector (AR: *:*:*) node
    sprintf(buf, "urn:publicid:IDN+%s+node+*", domainId.c_str());
    string arId = buf;
    Node* arNode = new Node(0, arId);
    aDomain->AddNode(arNode);

    //$$ get node info: rspec/stitching/aggregate/node
    for (xmlNode = aggrNode->children; xmlNode != NULL; xmlNode = xmlNode->next)
    {
        if (xmlNode->type == XML_ELEMENT_NODE )
        {
            if (strncasecmp((const char*)xmlNode->name, "node", 4) == 0) 
            {
                //$$create node
                xmlChar* xmlNodeId = xmlGetProp(xmlNode,  (const xmlChar*)"id");
                string nodeId = (const char*)xmlNodeId;
                Node* aNode = new Node(0, nodeId);
                aDomain->AddNode(aNode);
                //$$ get port info: rspec/stitching/aggregate/node/port
                xmlNodePtr xmlPortNode;
                for (xmlPortNode = xmlNode->children; xmlPortNode != NULL; xmlPortNode = xmlPortNode->next)
                {
                    if (xmlPortNode->type == XML_ELEMENT_NODE )
                    {
                        if (strncasecmp((const char*)xmlPortNode->name, "port", 4) == 0)
                        {
                            //$$ create port
                            xmlChar* xmlPortId = xmlGetProp(xmlPortNode,  (const xmlChar*)"id");
                            string portId = (const char*)xmlPortId;
                            Port* aPort = new Port(0, portId);
                            aNode->AddPort(aPort);
                            xmlNodePtr xmlLinkNode;
                            //$$ get link info: rspec/stitching/aggregate/node/port/link
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
                                                // TODO: parse SwitchingCapabilitityDescriptors in sub-level
                                            }
                                        }
                                        //$$create peering to AR (a. *:*--'to-nodename':* b. portname:*--"to-nodename-portname:*")
                                        // change remote-link-id on this link and create new port/link on AR
                                        string& remoteLinkName = aRLink->GetRemoteLinkName();
                                        size_t i1 = remoteLinkName.find("*:*:*");
                                        if (i1 != string::npos) 
                                        {
                                            string nodeShortName = GetUrnField(aNode->GetName(), "node");
                                            string portShortName = GetUrnField(aPort->GetName(), "port");
                                            sprintf(buf, "*:to-%s-%s:*", nodeShortName.c_str(), portShortName.c_str());
                                            remoteLinkName.replace(i1, 5, buf);
                                            aRLink->SetRemoteLinkName(remoteLinkName);
                                            string remotePortName = remoteLinkName;
                                            remotePortName.replace(remotePortName.size()-2, 2, "");
                                            Port* remotePort = new Port(0, remotePortName);
                                            arNode->AddPort(remotePort);
                                            RLink* remoteLink = new RLink(remoteLinkName);
                                            aPort->AddLink(remoteLink);
                                            remoteLink->SetRemoteLinkName(aRLink->GetName());
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



    //$$ get host info: rspec/node, rspec/node/interface, rspec/link, rspec/link/interface_ref
        //$$ recognize host interfaces attached to stitching links
                // not on a stitching host
                // link to a stitching port or link (or AR)
                // 
        //$$ create host as node; 
        //$$ create interface as port/link (link name = empty or *)
                // add link level to stitching side if not available; 
                // create peering by adding or modifying remote-link-id

    return xmlDoc;
}

// TODO: move RSpec related code to separate file/classes

