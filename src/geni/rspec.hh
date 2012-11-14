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

using namespace std;

class GeniRSpec {
protected:
    string rspecXml;
    xmlDocPtr rspecDoc;

public:
    GeniRSpec(): rspecDoc(NULL) { }
    GeniRSpec(string& xml): rspecXml(xml) { }
    GeniRSpec(xmlDocPtr doc): rspecDoc(doc) { }
    virtual ~GeniRSpec() { 
        if (rspecDoc != NULL)
            xmlFreeDoc(rspecDoc);
    }
};

class GeniAdRSpec: public GeniRSpec {
public:
    GeniAdRSpec(string& xml): GeniRSpec(xml) { }
    GeniAdRSpec(xmlDocPtr doc): GeniRSpec(doc) { }
    virtual ~GeniAdRSpec() { }
    xmlDocPtr TranslateToNML();
    // TODO: extract policy data
};

class GeniRequestRSpec: public GeniRSpec {
public:
    GeniRequestRSpec(string& xml): GeniRSpec(xml) { }
    GeniRequestRSpec(xmlDocPtr doc): GeniRSpec(doc) { }
    virtual ~GeniRequestRSpec() { }
};

class GeniManifestRSpec: public GeniRSpec {
public:
    GeniManifestRSpec(string& xml): GeniRSpec(xml) { }
    GeniManifestRSpec(xmlDocPtr doc): GeniRSpec(doc) { }
    virtual ~GeniManifestRSpec() { }  
};


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

#endif	/* __RSPEC_HH__ */

