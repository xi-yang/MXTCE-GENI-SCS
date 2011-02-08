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

#ifndef __TEDB_HH__
#define __TEDB_HH__

#include "event.hh"
#include "tewg.hh"
#include "resource.hh"
#include <libxml/tree.h>

using namespace std;

class TEWGLease
{
private:
    struct timeval checkTime;
    struct timeval dueTime;
    Resource* resource;

public:
    TEWGLease(): checkTime((struct timeval){0, 0}), dueTime((struct timeval){0, 0}), resource(NULL) { }
    ~TEWGLease() { }
    Resource* GetResource() { return resource; }
    void SetResource(Resource* r) { resource = r; }
    void Lend(Resource* r, int secs) {
        resource = r;
        dueTime.tv_sec = checkTime.tv_sec + secs;
        dueTime.tv_usec = checkTime.tv_usec ;
    }
    bool Expired() {
        struct timeval now;
        gettimeofday (&now, NULL);
        return (dueTime < now);        
    }
};


class DBDomain: public Domain
{
protected:
    struct timeval updateTime;
    xmlNodePtr xmlElem;
    list<TEWGLease> leases;

public:
    DBDomain(u_int32_t id, string& name): Domain(id, name), updateTime((struct timeval){0, 0}), xmlElem(NULL) { }
    DBDomain(u_int32_t id, string& name, string& address): Domain(id, name, address), updateTime((struct timeval){0, 0}), xmlElem(NULL) { }
    virtual ~DBDomain() { }
    xmlNodePtr GetXmlElement() { return xmlElem; }
    void SetXmlElement(xmlNodePtr x) { xmlElem = x; }
    void UpdateToXML(bool populateSubLevels=false);
    void UpdateFromXML(bool populateSubLevels=false);
    TDomain* Checkout(TGraph* tg);
};


class DBNode: public Node
{
protected:
    struct timeval updateTime;
    xmlNodePtr xmlElem;
    list<TEWGLease> leases;
    
public:
    DBNode(u_int32_t id, string& name): Node(id, name), updateTime((struct timeval){0, 0}), xmlElem(NULL) { }
    DBNode(u_int32_t id, string& name, string& address): Node(id, name, address), updateTime((struct timeval){0, 0}), xmlElem(NULL) { }
    virtual ~DBNode() { }
    xmlNodePtr GetXmlElement() { return xmlElem; }
    void SetXmlElement(xmlNodePtr x) { xmlElem = x; }            
    void UpdateToXML(bool populateSubLevels=false);
    void UpdateFromXML(bool populateSubLevels=false);
    TNode* Checkout(TGraph* tg);
};


class DBPort: public Port
{
protected:
    struct timeval updateTime;
    xmlNodePtr xmlElem;
    list<TEWGLease> leases;
        
public:
    DBPort(u_int32_t id, string& name): Port(id, name), updateTime((struct timeval){0, 0}), xmlElem(NULL) { }
    DBPort(u_int32_t id, string& name, string& address): Port(id, name, address), updateTime((struct timeval){0, 0}), xmlElem(NULL) { }
    virtual ~DBPort() { }
    xmlNodePtr GetXmlElement() { return xmlElem; }
    void SetXmlElement(xmlNodePtr x) { xmlElem = x; }            
    void UpdateToXML(bool populateSubLevels=false);
    void UpdateFromXML(bool populateSubLevels=false);
    TPort* Checkout(TGraph* tg);
};


class DBLink: public Link
{
protected:
    struct timeval updateTime;
    xmlNodePtr xmlElem;
    list<TEWGLease> leases;
            
public:
    DBLink(u_int32_t id, string& name): Link(id, name), updateTime((struct timeval){0, 0}), xmlElem(NULL) { }
    DBLink(u_int32_t id, string& name, string& address): Link(id, name, address), updateTime((struct timeval){0, 0}), xmlElem(NULL) { }
    virtual ~DBLink() { }
    xmlNodePtr GetXmlElement() { return xmlElem; }
    void SetXmlElement(xmlNodePtr x) { xmlElem = x; }            
    void UpdateToXML(bool populateSubLevels=false);
    void UpdateFromXML(bool populateSubLevels=false);
    TLink* Checkout(TGraph* tg);
};


class TEDB // with XML file persistent storage --> TODO MySQL DB storage
{
protected:
    string name;
    list<DBDomain*> dbDomains;
    list<DBNode*> dbNodes;
    list<DBPort*> dbPorts;
    list<DBLink*> dbLinks;
    
public:
    TEDB(string& n): name(n) { }
    virtual ~TEDB() { }

    //$$$$ DB orgnization methods --> TODO with xml importer
    //$$$$ insert /delete/ update ...

    //$$$$ XML I/O methods          --> TODO with xml importer

    TGraph* CheckoutTEWG(string& name); // full copy
    //TGraph* LeaseTEWG();  //partial copy
};


#endif
