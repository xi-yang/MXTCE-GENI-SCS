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

#include "vendor_specific.hh"
#include "exception.hh"

void VendorSpecificInfoParser::SetXmlByString(string& xml)
{
    xmlDocPtr xmlDoc = xmlParseMemory(xml.c_str(), xml.size());
    this->vendorSpecXmlNode = xmlDocGetRootElement(xmlDoc);
}


string VendorSpecificInfoParser::GetXmlByString()
{
    xmlChar* memBuf;
    const char* xmlBuf = "<xml/>";
    int sizeBuf=strlen(xmlBuf);
    if (this->vendorSpecXmlNode == NULL)
        return string("");
    xmlDocPtr xmlDoc = xmlParseMemory(xmlBuf, sizeBuf);
    xmlNodePtr oldXmlNode = xmlDocGetRootElement(xmlDoc);
    xmlDocSetRootElement(xmlDoc, this->vendorSpecXmlNode);
    xmlDocDumpMemory(xmlDoc, &memBuf, &sizeBuf);
    char* pStr = strstr((char*)memBuf, "?>");
    if (pStr == NULL)
        pStr = (char*)memBuf;
    else 
        pStr += 3;
    string xmlString = pStr; 
    xmlDocSetRootElement(xmlDoc, oldXmlNode);
    xmlFreeDoc(xmlDoc);
    xmlFree(memBuf);
    return xmlString;
}


VendorSpecificInfoParser* VendorSpecificInfoParserFactory::CreateParser(xmlNodePtr vendorSepcXmlNode)
{
    if (vendorSepcXmlNode == NULL || vendorSepcXmlNode->type != XML_ELEMENT_NODE)
        return NULL;

    xmlNodePtr xmlNode = NULL;
    if (vendorSepcXmlNode->type == XML_ELEMENT_NODE && strncasecmp((const char*)vendorSepcXmlNode->name, "infineraDTNSpecificInfo", 23) == 0)
    {
        for (xmlNode = vendorSepcXmlNode->children;  xmlNode != NULL; xmlNode = xmlNode->next)
        {
            if (xmlNode->type == XML_ELEMENT_NODE && strncasecmp((const char*)xmlNode->name, "tributaryInfo", 13) == 0)
                return (new VendorSpecificInfoParser_InfineraDTN_TributaryInfo(xmlNode));
            else if (xmlNode->type == XML_ELEMENT_NODE && strncasecmp((const char*)xmlNode->name, "wavebandMuxInfo", 15) == 0)
                return (new VendorSpecificInfoParser_InfineraDTN_WavebandMuxInfo(xmlNode));
        }
    }
    else if (vendorSepcXmlNode->type == XML_ELEMENT_NODE && strncasecmp((const char*)vendorSepcXmlNode->name, "cienaOTNSpecificInfo", 20) == 0)
    {
        return (new VendorSpecificInfoParser_CienaOTN(xmlNode));
    }

    return NULL;
}

void VendorSpecificInfoParser_InfineraDTN::Parse()
{
    xmlChar* attr;
    if (vendorSpecXmlNode->type == XML_ELEMENT_NODE && (strncasecmp((const char*)vendorSpecXmlNode->name, "tributaryInfo", 13) == 0
        || strncasecmp((const char*)vendorSpecXmlNode->name, "wavebandMuxInfo", 15) == 0))
    {
        this->type = "infineraDTNSpecificInfo:";
        this->type += (const char*)vendorSpecXmlNode->name;
        attr  = xmlGetProp(vendorSpecXmlNode, (const xmlChar*)"id");
        if (attr != NULL)
            this->id = (const char*)attr;
        attr  = xmlGetProp(vendorSpecXmlNode, (const xmlChar*)"model");
        if (attr != NULL)
            this->model = (const char*)attr;
        attr  = xmlGetProp(vendorSpecXmlNode, (const xmlChar*)"contain");
        if (attr != NULL)
        {
            int count; char type[16];
            int num = sscanf((const char*)attr, "%dx%s", &count, type);
            if (num == 2)
            {
                this->containType = (const char*)type;
                this->containCount = count;
            }
            else
            {
                this->containType = (const char*)attr;
                this->containCount = 1;
            }
        }
        return;
    }
    throw TEDBException((char*)"No valid data contained in 'infineraDTNSpecificInfo' -- require either 'tributaryInfo' or 'wavebandMuxInfo'");   
}


OTNObject::~OTNObject()
{
    for (int i = 0; i < this->containObjects.size(); i++)
        delete containObjects[i];
    containObjects.clear();
}

void OTNObject::Parse()
{
    xmlNodePtr xmlNode;
    xmlChar* attr;
    if (vendorSpecXmlNode->type == XML_ELEMENT_NODE)
    {
        this->containObjects.clear();
        if (strncasecmp((const char*)vendorSpecXmlNode->name, "OTU", 3) != 0 &&
            strncasecmp((const char*)vendorSpecXmlNode->name, "OCh", 3) != 0 &&
            strncasecmp((const char*)vendorSpecXmlNode->name, "OCG", 3) != 0)
        {
            char buf[128];
            snprintf(buf, 128, "Unrecognized OTNObject type: %s", (const char*)vendorSpecXmlNode->name);
            throw TEDBException(buf);   
        }
        this->type = (const char*)vendorSpecXmlNode->name;
        attr  = xmlGetProp(vendorSpecXmlNode, (const xmlChar*)"id");
        if (attr != NULL)
            this->id = (const char*)attr;
        attr  = xmlGetProp(vendorSpecXmlNode, (const xmlChar*)"model");
        if (attr != NULL)
            this->model = (const char*)attr;
        attr  = xmlGetProp(vendorSpecXmlNode, (const xmlChar*)"contain");
        if (attr != NULL)
        {
            int count; char type[16];
            int num = sscanf((const char*)attr, "%dx%s", &count, type);
            if (num == 2)
            {
                this->containType = (const char*)type;
            }
            else
            {
                this->containType = (const char*)attr;
                count = 0;
            }
            for (xmlNode = vendorSpecXmlNode->children; xmlNode != NULL; xmlNode = xmlNode->next)
            {
                if (xmlNode->type == XML_ELEMENT_NODE )
                {
                    if (strncasecmp((const char*)xmlNode->name, "OTU", 3) != 0 &&
                        strncasecmp((const char*)xmlNode->name, "OCh", 3) != 0 &&
                        strncasecmp((const char*)xmlNode->name, "OCG", 3) != 0)
                    {
                        continue;
                    }
                    OTNObject* oobj = new OTNObject(xmlNode);
                    oobj->Parse();
                    this->containObjects.push_back(oobj);
                }
            }
            if (this->containObjects.size() == 0 && count == 1)
            {
                OTNObject* oobj = new OTNObject(this->containType);
                this->containObjects.push_back(oobj);
            }
            else if (this->containObjects.size() != count)
            {
                char buf[128];
                snprintf(buf, 128, "Malformed '%s' data structure: requires %d sub-level objects, actually contains %d", 
                    this->type.c_str(), count, (int)this->containObjects.size());
                throw TEDBException(buf);
            }
        }
        return;
    }
    throw TEDBException((char*)"Parsing OTNObject: not a valid XML node");   
}

void VendorSpecificInfoParser_InfineraDTN_TributaryInfo::Parse()
{
    VendorSpecificInfoParser_InfineraDTN::Parse();
    xmlNodePtr xmlNode;
    for (xmlNode = vendorSpecXmlNode->children; xmlNode != NULL; xmlNode = xmlNode->next)
    {
        if (xmlNode->type == XML_ELEMENT_NODE )
        {
            if (strncasecmp((const char*)vendorSpecXmlNode->name, "OTU", 3) != 0)
            {
                tribOTU = new OTNObject(xmlNode);
                tribOTU->Parse();
                return;
               }
        }
    }
    throw TEDBException((char*)"Parsing InfineraDTN_TributaryInfo: XML not containing an OTUx type OTNObject");   
}

void VendorSpecificInfoParser_InfineraDTN_WavebandMuxInfo::Parse()
{
    VendorSpecificInfoParser_InfineraDTN::Parse();
    if (strncasecmp(this->containType.c_str(), "OCG", 3) != 0)
        throw TEDBException((char*)"Parsing InfineraDTN_WavebandMuxInfo: requires 'NxOCG' as 'contain' atribute");
    xmlNodePtr xmlNode;
    this->ocgVector.clear();
    for (xmlNode = vendorSpecXmlNode->children; xmlNode != NULL; xmlNode = xmlNode->next)
    {
        if (xmlNode->type == XML_ELEMENT_NODE )
        {
            if (strncasecmp((const char*)vendorSpecXmlNode->name, "OCG", 3) != 0)
            {
                OTNObject* ocg = new OTNObject(xmlNode);
                ocg->Parse();
                this->ocgVector.push_back(ocg);
            }
        }
    }
    if (this->ocgVector.size() != this->containCount)
    {
        char buf[128];
        snprintf(buf, 128, "Malformed InfineraDTN_WavebandMuxInfo data structure: requires %d OCG objects, actually contains %d", 
            this->containCount, (int)this->ocgVector.size());
        throw TEDBException(buf);
    }
}


void VendorSpecificInfoParser_CienaOTN::Parse()
{
    // TODO: 
}
