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


#ifndef __RSPEC_HH__
#define	__RSPEC_HH__

#include "types.hh"
#include "resource.hh"
#include "workflow.hh"
#include "xmlrpc_apiserver.hh"
#include <map>

using namespace std;

class GeniRSpec {
protected:
    string rspecXml;
    xmlDocPtr rspecDoc;
    //embedded class definition
    class RLink: public Link
    {
    protected:
        string remoteLinkName;
        string swcapXmlString;

    public:
        RLink(string& name): Link(0, name) { }
        virtual ~RLink() { }
        void SetRemoteLinkName(string& name) { remoteLinkName = name; }
        string& GetRemoteLinkName() { return remoteLinkName; }
        void SetSwcapXmlString(string& xml) { swcapXmlString = xml; }
        string& GetSwcapXmlString() { return swcapXmlString; }
    };

public:
    GeniRSpec(): rspecDoc(NULL), rspecXml("") { }
    GeniRSpec(string& xml): rspecXml(xml), rspecDoc(NULL) { }
    GeniRSpec(xmlDocPtr doc): rspecDoc(doc), rspecXml("") { }
    virtual ~GeniRSpec() { 
        if (rspecDoc != NULL)
            xmlFreeDoc(rspecDoc);
    }
    string& GetRspecXmlString() { return rspecXml; }
    void SetRspecXmlString(string& xml) { rspecXml = xml; }
    xmlDocPtr GetRspecXmlDoc() { return rspecDoc; }
    void SetRspecXmlDoc(xmlDocPtr doc) { rspecDoc = doc; }
    void ParseRspecXml();
    void DumpRspecXml();
    
    // format: 20120815:09:30:00
    void GeniTimeString(char* buf)
    {
      time_t rawtime;
      struct tm * timeinfo;

      time (&rawtime);
      timeinfo = localtime ( &rawtime );

      strftime (buf, 20,"%Y%m%d:%H:%M:%S",timeinfo);
    }
};


class GeniAdRSpec: public GeniRSpec {
public:
    GeniAdRSpec(string& xml): GeniRSpec(xml) { }
    GeniAdRSpec(xmlDocPtr doc): GeniRSpec(doc) { }
    virtual ~GeniAdRSpec() { }
    xmlDocPtr TranslateToNML();
    // TODO: extract policy data
public:
    static map<string, string> aggregateUrnMap;
    static map<string, string> aggregateUrlMap;
    static map<string, string> aggregateTypeMap;
};

class Message;
class Apimsg_user_constraint;
class GeniRequestRSpec: public GeniRSpec {
protected:
    map<string, Apimsg_user_constraint*> cachedUserConstraints;

public:
    static int unique_req_id;
    GeniRequestRSpec(string& xml): GeniRSpec(xml) { }
    GeniRequestRSpec(xmlDocPtr doc): GeniRSpec(doc) { }
    virtual ~GeniRequestRSpec();
    Apimsg_user_constraint* GetUserConstraintByPathId(string& id);
    Message* CreateApiRequestMessage(map<string, xmlrpc_c::value>& rp);
};

class GeniManifestRSpec: public GeniRSpec {
protected:
    GeniRequestRSpec* pairedRequestRspec;
    map<string, WorkflowData*> workflowDataMap;

public:
    GeniManifestRSpec(): pairedRequestRspec(NULL) { }
    GeniManifestRSpec(string& xml): GeniRSpec(xml), pairedRequestRspec(NULL) { }
    GeniManifestRSpec(xmlDocPtr doc): GeniRSpec(doc), pairedRequestRspec(NULL) { }
    GeniManifestRSpec(GeniRequestRSpec* reqRspec): pairedRequestRspec(reqRspec) { }
    virtual ~GeniManifestRSpec() { }
    void ParseApiReplyMessage(Message* msg);
    map<string, WorkflowData*>& GetWorkflowDataMap() { return workflowDataMap; }
};

#define GENI_PCS_ERRCODE_NO_ERROR 0x0
#define GENI_PCS_ERRCODE_MAILFORMED_REQUEST 0x0001
#define GENI_PCS_ERRCODE_TIMEOUT 0x0002
#define GENI_PCS_ERRCODE_INCOMPLETE_REPLY 0x0003
#define GENI_PCS_ERRCODE_UNKNOWN 0xFFFF

#endif	/* __RSPEC_HH__ */

