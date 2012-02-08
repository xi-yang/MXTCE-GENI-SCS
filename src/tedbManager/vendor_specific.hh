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

#ifndef __VENDOR_SPECIFIC_HH__
#define __VENDOR_SPECIFIC_HH__

#include "types.hh"
#include "utils.hh"
#include <vector>

using namespace std;

class VendorSpecificInfoParser
{
protected:
    string type;
    xmlNodePtr vendorSpecXmlNode;

public:
    VendorSpecificInfoParser(xmlNodePtr xmlNode): vendorSpecXmlNode(xmlNode) { }
    virtual ~VendorSpecificInfoParser() { }
    string& GetType() {return type;}
    virtual void Parse()=0;
    // TODO: clone method
};

class VendorSpecificInfoParserFactory
{
protected:
    // TODO: maintain a map between xmlNode (or link urn?) =and parser
public:
    VendorSpecificInfoParserFactory() { }
    virtual ~VendorSpecificInfoParserFactory() { }
    static VendorSpecificInfoParser* CreateParser(xmlNodePtr xmlNode);
};


class VendorSpecificInfoParser_InfineraDTN: public VendorSpecificInfoParser
{
protected:
    string id;
    string model;
    string containType;
    int containCount;

public:
    VendorSpecificInfoParser_InfineraDTN(xmlNodePtr xmlNode): VendorSpecificInfoParser(xmlNode), containCount(0) { }
    virtual ~VendorSpecificInfoParser_InfineraDTN() { }
    string& GetId() {return id;}
    string& GetModel() {return model;}
    string& GetContainsType() {return containType;}
    int GetContainsCount() {return containCount;}
    virtual void Parse();
};

class OTNObject
{
protected:
    string type;
    string id;
    string model;
    string containType;
    vector<OTNObject*> containObjects; // size 0: no lower hierarchy (base OTUx or payload type) 
    xmlNodePtr vendorSpecXmlNode;

public:
    OTNObject(): vendorSpecXmlNode(NULL) { }
    OTNObject(xmlNodePtr xmlNode): vendorSpecXmlNode(xmlNode) { }
    OTNObject(string t): type(t), vendorSpecXmlNode(NULL) { }
    virtual ~OTNObject();
    string& GetType() {return type;}
    string& GetId() {return id;}
    string& GetModel() {return model;}
    string& GetContainType() {return containType;}
    vector<OTNObject*>& GetContainObjects() {return containObjects;}
    virtual void Parse();
    // TODO: clone method
};

class VendorSpecificInfoParser_InfineraDTN_TributaryInfo: public VendorSpecificInfoParser_InfineraDTN
{
private:
    OTNObject* tribOTU;

public:
    VendorSpecificInfoParser_InfineraDTN_TributaryInfo(xmlNodePtr xmlNode): VendorSpecificInfoParser_InfineraDTN(xmlNode), tribOTU(NULL) { }
    virtual ~VendorSpecificInfoParser_InfineraDTN_TributaryInfo() { delete tribOTU; }
    virtual void Parse();
};

class VendorSpecificInfoParser_InfineraDTN_WavebandMuxInfo: public VendorSpecificInfoParser_InfineraDTN
{
private:
    vector<OTNObject*> ocgVector;

public:
        VendorSpecificInfoParser_InfineraDTN_WavebandMuxInfo(xmlNodePtr xmlNode): VendorSpecificInfoParser_InfineraDTN(xmlNode) { }
        virtual ~VendorSpecificInfoParser_InfineraDTN_WavebandMuxInfo() {
            for (int i = 0; i < ocgVector.size(); i++)
                delete ocgVector[i];
            ocgVector.clear();
        }
    virtual void Parse();
};

// place holder
class VendorSpecificInfoParser_CienaOTN: public VendorSpecificInfoParser
{
protected:
public:
    VendorSpecificInfoParser_CienaOTN(xmlNodePtr xmlNode): VendorSpecificInfoParser(xmlNode) { }
    virtual ~VendorSpecificInfoParser_CienaOTN() { }
    virtual void Parse();
};

#endif
