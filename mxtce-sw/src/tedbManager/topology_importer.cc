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
#include "rspec.hh"


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
            GeniAdRSpec rspec(xmlDoc);
            xmlDoc = rspec.TranslateToNML();
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

