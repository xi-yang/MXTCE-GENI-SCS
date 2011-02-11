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


long StringToBandwidth(string strBandwidth) 
{
    long bandwidth = 0L;
    long factor = 1;
    if (strcasestr(strBandwidth.c_str(), "K"))
    {
        factor = 1000;
    }
    if (strcasestr(strBandwidth.c_str(), "M"))
    {
        factor = 1000000;
    }
    if (strcasestr(strBandwidth.c_str(), "G"))
    {
        factor = 1000000000;
    }

    if (sscanf(strBandwidth.c_str(), "%ld", & bandwidth) == 1)
        return bandwidth*factor;
    return 0;
}


string GetUrnField(string urn, const char* field) 
{
    string name;
    char str[128];
    snprintf(str, 128, "%s=", field);
    char* ptr = strstr(urn.c_str(), str);
    if (ptr == NULL)
        return NULL;
    ptr += (strlen(field)+1);
    char* ptr2 = strstr(ptr, ":"); 
    if (ptr2 == NULL)
        name = (const char*)ptr;
    else 
    {
        strncpy(str, ptr, ptr2-ptr);
        name = (const char*)str;
    }
    return name;
}

