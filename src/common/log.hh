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

#ifndef __LOG_HH__
#define __LOG_HH__
#include <iostream>
#include <fstream>
#include "types.hh"
#include "thread.hh"

using namespace std;


typedef enum 
{
    LOG_SILENCE =0,
    LOG_ERROR = 0x1,
    LOG_WARNING = 0x2,
    LOG_DEBUG = 0x4,
    LOG_APPEND = 0x8,
    LOG_LOGFILE = 0x10,
    LOG_STDOUT = 0x20,
    LOG_STDERR = 0x40,
    LOG_ALL = 0xff
} LogOption;

/////////////Log  (A singleton class)/////////////
class Lock;
class Log
{
private:
    static Log logger;
    static bool blDebug;
    static string more_info;
    Log() {}

public:
    ~Log() {
        if (log_file) 
        {
            log_file->flush(); 
            delete log_file;
        } 
        if (log_stdout) 
            log_stdout->flush();
        if (log_stderr)
            log_stderr->flush();
    }

    static u_int32_t options;
    static ostream* log_file;
    static ostream* log_stdout;
    static ostream* log_stderr;
    static Lock* loggerLock;
 
    static bool Debug() {return blDebug;}
    static void SetDebug(bool val) {blDebug = val;}
    static void SetPrompt (string str) {more_info = str;}
    static Log& Instance() {return logger;}
    static void Init(u_int32_t options_val, const string& fileName);
    static const string Preamble(LogOption option);
    static void EnableOption(LogOption opt) {options |= opt;}
    static void DisableOption(LogOption opt) {options &= (~opt);}
    static int Logf(const char *format, ...);
};


#define LOG_FILE   (*Log::log_file<<Log::Preamble(LOG_LOGFILE))
#define LOG_COUT   (*Log::log_stdout<<Log::Preamble(LOG_STDOUT))
#define LOG_CERR   (*Log::log_stderr<<Log::Preamble(LOG_STDERR))
#define LOG(X)     Log::loggerLock->DoLock(); \
                   if (Log::options&LOG_LOGFILE && Log::options&LOG_STDOUT) \
                      { LOG_FILE<<X<<flush; LOG_COUT<<X<<flush; }  \
                   else if (Log::options&LOG_LOGFILE && Log::options&LOG_STDERR) \
                      { LOG_FILE<<X<<flush; LOG_CERR<<X<<flush; }  \
                   else if (Log::options&LOG_LOGFILE) LOG_FILE<<X<<flush; \
                   else if (Log::options&LOG_STDOUT) LOG_COUT<<X<<flush;  \
                   else if (Log::options&LOG_STDERR) LOG_CERR<<X<<flush;  \
                   Log::loggerLock->Unlock();

#define LOGF Log::Logf
#define LOG_DEBUG(X) if(Log::Debug()) LOG(X)
#define LOG_DEBUGF(X) if(Log::Debug()) LOGF(X)

#endif
