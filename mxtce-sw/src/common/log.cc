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
#include <stdarg.h>

// Global variable - the only instance of Log.
Log Log::logger;
ostream* Log::log_file = (ostream*)NULL;
u_int32_t Log::options = 0; 
ostream* Log::log_stdout = &cout;
ostream* Log::log_stderr = &cerr;
Lock* Log::loggerLock = NULL;
bool Log::blDebug = false;
string Log::more_info = "";

const string Log::Preamble(LogOption option)
{
    string s;
   
    time_t t;
    time(&t);
    tm* ltime = localtime(&t);

    switch (option)
    {
    case LOG_LOGFILE:
        s = "MXTCE ";
        break;
    case LOG_STDOUT:
        s = "MXTCE_OUTPUT ";
        break;
    case LOG_STDERR:
        s = "MXTCE_ERROR ";
        break;
    case LOG_DEBUG:
        s = "MXTCE_DEBUG ";
        break;
    default:
        s ="?MXTCE_LOG? ";
    }

    static char buf[30];    
    strftime (buf, 30, "%m/%d %H:%M:%S ", ltime);
    s += buf;

    if (!more_info.empty())
    {
        s += "<";
        s += more_info;
        s += "> : ";
    }
    
    return s;
}

void Log::Init(u_int32_t options_val, const string &fileName)
{   
    options = options_val;
    loggerLock = new Lock();
    if (fileName.empty())
        return;
    log_file = new ofstream(fileName.c_str(), ios::out |((options_val & LOG_APPEND) ? ios::app : ios::trunc) );
    if (!log_file || log_file->bad()) 
    {
        LOG_CERR << "Failed to open the log file: " << fileName << endl;
        LOG_CERR << "Logging to stdout instead." << endl;
        log_stderr = &cout;
    }
}

int Log::Logf(const char *format, ...)
{
    static char buf[256*1024];

    loggerLock->DoLock();

    buf[0] = 0;
    va_list ap;
    va_start(ap, format);
    int ret=vsprintf(buf, format, ap);
    if (log_file && (options&LOG_LOGFILE))
        *log_file<< Preamble(LOG_LOGFILE) << buf<<flush;
    if (log_stdout && (options&LOG_STDOUT))
        *log_stdout<<Preamble(LOG_LOGFILE) << buf<<flush;
    va_end(ap);

    loggerLock->Unlock();
    return ret;
}

