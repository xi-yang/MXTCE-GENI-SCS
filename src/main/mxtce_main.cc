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

#include "types.hh"
#include "log.hh"
#include "event.hh"
#include "utils.hh"
#include "exception.hh"
#include "mxtce.hh"

#ifndef HAVE_DECL_GETOPT
  #define HAVE_DECL_GETOPT 1
#endif
#include <getopt.h>

struct option longopts[] = 
{
    { "daemon",      no_argument,       NULL, 'd'},
    { "config_file", required_argument, NULL, 'c'},
    { "api_port",    required_argument, NULL, 'p'},
    { "help",        no_argument,       NULL, 'h'},
    { 0 }
};

void showUsage()
{
    cout<<"MX-TCE Usage:"<<endl;
    cout<<"\t mxtce [-d] (daemon)  [-c config_file] [-p API port] [-h] (help)" <<endl;
}

//extern const char* MxTCEVersionString();


int runAsDaemon (int nochdir, int noclose)
{
  pid_t pid;

  pid = fork ();

  if (pid < 0)
	{
	  perror ("fork");
	  return -1;
	}

  if (pid != 0)
	exit (0);

  pid = setsid();

  if (pid < -1)
	{
	  perror ("setsid");
	  return -1;
	}

  if (! nochdir)
	chdir ("/");

  if (! noclose)
	{
	  int fd;

	  fd = open ("/dev/null", O_RDWR, 0);
	  if (fd != -1)
	{
	  dup2 (fd, STDIN_FILENO);
	  dup2 (fd, STDOUT_FILENO);
	  dup2 (fd, STDERR_FILENO);
	  if (fd > 2)
		close (fd);
	}
	}

  umask (0027);

  return 0;
}


int main( int argc, char* argv[])
{
    bool is_daemon = false;
    LogOption log_opt = LOG_ALL;
    const char* configfile = "/usr/local/etc/mxtce.config.yaml";

    while (1)
    {
        int opt;

        opt = getopt_long (argc, argv, "dc:p:h", longopts, 0);
        if (opt == EOF)
            break;

        switch (opt) 
        {
        case 'c':
            configfile = optarg;
            break;
        case 'd':
            is_daemon = true;
            break;
        case 'p':
            MxTCE::apiServerPort = atoi(optarg);
            break;
        case 'h':
        default:
            showUsage();
            break;
        }
    }

    signal_init ();

    // init logging
    char log_file[20];
    sprintf(log_file, "/var/log/mxtce.log");    
    Log::Init(log_opt, log_file);
    Log::SetDebug(true);
    LOG(endl<<endl<<"#####################"<<endl
        <<"MxTCE Started..."<<endl
    //    <<MxTCEVersionString()<<endl
        <<"#####################"<<endl<<endl);

    // init daemon mode
    if (is_daemon)
        runAsDaemon(0, 0);

    // $$$$  Start core MxTCE thread
    MxTCE* mxTCECore = new MxTCE(configfile);
    try {
        mxTCECore->Start();
    } catch (TCEException e) {
        cerr << "MxTCE core threw Exception: " << e.GetMessage() << endl;
        LOG("MxTCE core threw Exception: " << e.GetMessage() <<endl);
    }

    //coreEventMaster->Run(); //moved to MxTCE core (MxTCE::Run)
    return 0;
}


