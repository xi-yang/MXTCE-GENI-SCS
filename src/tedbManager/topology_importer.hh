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

#ifndef __TOPOLOGY_IMPORTER_HH__
#define __TOPOLOGY_IMPORTER_HH__

#include "utils.hh"
#include "event.hh"
#include "tedb.hh"

class TopologyXMLImporter: public Timer
{
protected:
    TEDB* tedb;
    list<string> xmlFilePathList;
    list<time_t> fileModTimeList;
    void Init();

public:
    TopologyXMLImporter(TEDB* db, list<string>& fileList): tedb(db){ xmlFilePathList.assign(fileList.begin(), fileList.end()); Init(); }
    TopologyXMLImporter(TEDB* db, list<string>& fileList, int interval): Timer(interval, 0, FOREVER), tedb(db) { xmlFilePathList.assign(fileList.begin(), fileList.end()); Init(); }
    virtual ~TopologyXMLImporter() { }
    virtual void Run();
    string CheckFileType(xmlDocPtr xmlDoc);
    xmlDocPtr TranslateFromRspec(xmlDocPtr rspecDoc);
};

class RLink: public Link
{
protected:
    string remoteLinkName;

public:
    RLink(string& name): Link(0, name) { }
    void SetRemoteLinkName(string name) { remoteLinkName = name; }
    string& GetRemoteLinkName() { return remoteLinkName; }
};

#endif
