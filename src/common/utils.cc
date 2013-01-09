/*
 * Copyright (c) 2010-2011
 * ARCHSTONE Project.
 * University of Southern California/Information Sciences Institute.
 * All rights reserved.
 *
 * Created by Xi Yang 2010
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

#include "log.hh"
#include "utils.hh"
#include <signal.h>
#include <libxml2/libxml/xpath.h>

int readn (int fd, char *ptr, int nbytes)
{
    int nleft;
    int nread;

    nleft = nbytes;
    while (nleft > 0) 
    {
        nread = read (fd, ptr, nleft);
        if (nread < 0) 
            return (nread);
        else
            if (nread == 0) 
                break;

        nleft -= nread;
        ptr += nread;
    }

    return nbytes - nleft;
}  


int writen(int fd, char *ptr, int nbytes)
{
	int nleft;
	int nwritten;

	nleft = nbytes;
	while (nleft > 0) 
	{
        nwritten = write(fd, ptr, nleft);

        if (nwritten <= 0) 
            return (nwritten);

        nleft -= nwritten;
        ptr += nwritten;
    }
    return nbytes - nleft;
}

// SIGHUP handler
void sighup (int sig)
{
    LOG("SIGHUP received"<<endl);
}

void sigint (int sig)
{
    LOG("Terminating on signal SIGINT" << endl);
    exit(0);
}
void sigsegv (int sig)
{
    LOG("Terminating on signal SIGSEGV" << endl);
    exit(0);
}

// Signal wrapper.
RETSIGTYPE * signal_set (int signo, void (*func)(int))
{
    int ret;
    struct sigaction sig;
    struct sigaction osig;
  
    sig.sa_handler = func;
    sigemptyset (&sig.sa_mask);
    sig.sa_flags = 0;
#ifdef SA_RESTART
    sig.sa_flags |= SA_RESTART;
#endif
  
    ret = sigaction (signo, &sig, &osig);
  
    if (ret < 0)
        return ((RETSIGTYPE*)SIG_ERR);
    else
        return ((RETSIGTYPE*)osig.sa_handler);
}

// Initialization of signal handles.
void signal_init ()
{
    signal_set (SIGHUP, sighup);
    signal_set (SIGINT, sigint);
    signal_set (SIGTERM, sigint);
    //signal_set (SIGSEGV, sigsegv);
    signal_set (SIGPIPE, SIG_IGN);
#ifdef SIGTSTP
    signal_set (SIGTSTP, SIG_IGN);
#endif
#ifdef SIGTTIN
    signal_set (SIGTTIN, SIG_IGN);
#endif
#ifdef SIGTTOU
    signal_set (SIGTTOU, SIG_IGN);
#endif
}


static int do_mkdir(const char *path, mode_t mode)
{    
    struct stat st;    
    int status = 0;
    if (stat(path, &st) != 0)    
    {        /* Directory does not exist */        
        if (mkdir(path, mode) != 0)            
            status = -1;    
    }    
    else if (!S_ISDIR(st.st_mode))    
    {        
        errno = ENOTDIR;        
        status = -1;    
    }   
    return(status);
}

int mkpath(const char *path, mode_t mode)
{    
    char *pp;
    char *sp;    
    int  status;   
    char *copypath = strdup(path);    
    status = 0;    
    pp = copypath;    
    while (status == 0 && (sp = strchr(pp, '/')) != 0)    
    {        
        if (sp != pp)        
        {            
            /* Neither root nor double slash in path */            
            *sp = '\0';            
            status = do_mkdir(copypath, mode);            
            *sp = '/';        
        }        

        pp = sp + 1;    

    }    
    if (status == 0)       
        status = do_mkdir(path, mode);    
    free(copypath);    
    return (status);
}



time_t get_mtime(const char *path)
{
    struct stat statbuf;
    if (stat(path, &statbuf) == -1) {
        return 0;
    }
    return statbuf.st_mtime;
}


// C1 = 1, H1 = 2 ...
int wavegrid_50g_to_tag(char ch, int num)
{
    int half = 0;
    if (ch=='h' || ch=='H')
        half = 1;
    return ((num)*2+half-1);
}

void wavegrid_tag_to_50g(char* buf, int tag)
{
    if (tag%2==0)
        snprintf(buf, 4, "H%d", (tag+1)/2);
    else
        snprintf(buf, 4, "C%d", (tag+1)/2);
}


void StripXmlString(string& str, xmlChar* val) 
{
    char cstr[512];
    if (val == NULL || (*val) == 0)
    {
        str = "";
        return;
    }
    strncpy(cstr, (const char*)val, 512);
    char* ptr = cstr;
    while ((*ptr) && isspace(*ptr))
        ptr++;
    char* ptr2 = cstr + strlen(cstr)-1;
    while (ptr2 >= ptr && isspace(*ptr2))
        ptr2--;
    if (ptr > ptr2)
    {
        str = "";
        return;
    }
    ptr2++;
    *ptr2 = 0;
    str = (const char*) ptr;
}

void CleanupXmlString(string& str) 
{
    string::size_type pos = 0;
    string str1="    ";
    string str2="";
    while ( (pos = str.find(str1, pos)) != string::npos ) 
    {
        str.replace( pos, str1.size(), str2 );
        pos++;
    }
    str1 = "\t";
    while ( (pos = str.find(str1, pos)) != string::npos ) 
    {
        str.replace( pos, str1.size(), str2 );
        pos++;
    }
    str1 = "\n";
    while ( (pos = str.find(str1, pos)) != string::npos ) 
    {
        str.replace( pos, str1.size(), str2 );
        pos++;
    }
}

u_int64_t StringToBandwidth(string& strBandwidth) 
{
    u_int64_t bandwidth = 0LLU;
    u_int64_t factor = 1LLU;
    if (strcasestr(strBandwidth.c_str(), "K"))
    {
        factor = 1000LLU;
    }
    if (strcasestr(strBandwidth.c_str(), "M"))
    {
        factor = 1000000LLU;
    }
    if (strcasestr(strBandwidth.c_str(), "G"))
    {
        factor = 1000000000LLU;
    }

    if (sscanf(strBandwidth.c_str(), "%llu", &bandwidth) == 1)
        return bandwidth*factor;
    return 0;
}


string GetUrnField(string& urn, const char* field) 
{
    string name = "";
    char str[128];
    snprintf(str, 128, "%s=", field);
    char* ptr = (char*)strstr(urn.c_str(), str);
    if (ptr == NULL)
    {
        string domain, node, port, link;
        ParseFQUrnShort(urn, domain, node, port, link);
        if (strncmp(field, "domain", 4) == 0 || strncmp(field, "aggregate", 4) == 0)
            return domain;
        else if (strncmp(field, "node", 4) == 0)
            return node;
        else if (strncmp(field, "port", 4) == 0)
            return port;
        else if (strncmp(field, "link", 4) == 0)
            return link;
        else
            return name;
    }
    ptr += (strlen(field)+1);
    char* ptr2 = strstr(ptr, ":"); 
    if (ptr2 == NULL)
        name = (const char*)ptr;
    else 
    {
        strncpy(str, ptr, ptr2-ptr);
        str[ptr2-ptr] = 0;
        name = (const char*)str;
    }
    return name;
}

string ConvertLinkUrn_Dnc2Geni(string& urn) 
{
    string domain, node, port, link;
    string geniUrn = "urn:publicid:IDN+";
    ParseFQUrn(urn, domain, node, port, link);
    geniUrn += domain;
    geniUrn += "+interface+";
    geniUrn += node;
    geniUrn += ":";
    geniUrn += port;
    if (!link.empty())
    {
        geniUrn += ":";
        geniUrn += link;
    }
    return geniUrn;
}

void ParseFQUrn(string& urn, string& domain, string& node, string& port, string& link) 
{
    domain = GetUrnField(urn, "domain");
    node = GetUrnField(urn, "node");
    port = GetUrnField(urn, "port");
    link = GetUrnField(urn, "link");
    if (domain.length() == 0 && node.length() == 0 && port.length() == 0 && link.length() == 0)
        ParseFQUrnShort(urn, domain, node, port, link);
}

// example nml urn: 'urn:ogf:network:es.net:fnal-mr2:xe-7/0/0:*'
// example geni urn: 'urn:publicid:IDN+ion.internet2.edu+stitchport+rtr.newy:xe-0/0/3'
// example geni urn: 'urn:publicid:IDN+ion.internet2.edu+interface+rtr.newy:xe-0/0/3:*'
// example geni urn: 'urn:publicid:IDN+ion.internet2.edu+node+rtr.newy'
void ParseFQUrnShort(string& urn, string& domain, string& node, string& port, string& link) 
{
    char buf[256];
    char *pbuf=buf, *ps=NULL;
    if (strncmp(urn.c_str(), "urn:ogf:network:", 16) == 0) 
    {
        strncpy(buf, urn.c_str()+16, 256);
        ps = strstr(pbuf, ":");
        if (ps != NULL)
        {
            *ps = 0;
        }
        else 
            return;
        domain = pbuf;
        pbuf = ps+1;
        ps = strstr(pbuf, ":");
        if (ps != NULL)
        {
            *ps = 0;
        }
        else 
            return;
        node = pbuf;
        pbuf = ps+1;
        ps = strstr(pbuf, ":");
        if (ps != NULL)
        {
            *ps = 0;
        }
        else 
            return;
        port = pbuf;
        pbuf = ps+1;
        ps = strstr(pbuf, ":");
        if (ps != NULL)
        {
            return;
        }
        link = pbuf;        
    } else if (strncmp(urn.c_str(), "urn:publicid:IDN+", 17) == 0) 
    {
        string type;
        strncpy(buf, urn.c_str()+17, 256);
        ps = strstr(pbuf, "+");
        if (ps != NULL)
        {
            *ps = 0;
        }
        else 
            return;
        domain = pbuf;
        pbuf = ps+1;
        ps = strstr(pbuf, "+");
        if (ps != NULL)
        {
            *ps = 0;
        }
        else 
            return;
        type = pbuf;
        pbuf = ps+1;
        ps = strstr(pbuf, ":");
        if (ps != NULL)
        {
            *ps = 0;
        }
        else 
        {
            node = pbuf;
            return;
        }
        node = pbuf;
        pbuf = ps+1;
        ps = strstr(pbuf, ":");
        if (ps != NULL)
        {
            *ps = 0;
        }
        else 
        {
            port = pbuf;
            return;
        }
        port = pbuf;
        pbuf = ps+1;
        ps = strstr(pbuf, ":");
        link = pbuf;        
    }
}

xmlNodeSetPtr GetXpathNodeSet (xmlDocPtr doc, const char *xpath)
{
    xmlXPathContextPtr context;
    xmlXPathObjectPtr result;

    context = xmlXPathNewContext(doc);
    if (context == NULL) 
    {
        return NULL;
    }
    result = xmlXPathEvalExpression((xmlChar*)xpath, context);
    xmlXPathFreeContext(context);
    if (result == NULL) 
    {
        return NULL;
    }
    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) 
    {
        xmlXPathFreeObject(result);
        return NULL;
    }
    return result->nodesetval;
}


xmlNodePtr GetXpathNode (xmlDocPtr doc, const char *xpath)
{
    xmlXPathContextPtr context;
    xmlNodeSetPtr nodeset = GetXpathNodeSet(doc, xpath);

    if (nodeset == NULL) 
    {
        return NULL;
    }
    return nodeset->nodeTab[0];
}