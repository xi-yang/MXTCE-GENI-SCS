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


#ifndef __EXCEPTION_HH__
#define __EXCEPTION_HH__

#include <string>
#include <exception>

using namespace std;

class TCEException: public exception
{
protected:
    string errMessage;
public:
    TCEException(string msg): errMessage(msg){ }
    virtual ~TCEException()throw(){ }  
    virtual const char* what() const throw() {
        return (const char*)"TCEException";
    }
    virtual string& GetMessage()
    {
        return errMessage;
    }
};

class MsgIOException: public TCEException
{
public:
    MsgIOException(string msg): TCEException(msg){ }
    virtual ~MsgIOException()throw(){ }  
    virtual const char* what() const throw() {
        return (const char*)"MsgIOException";
    }
};

class ThreadException: public TCEException
{
public:
    ThreadException(string msg): TCEException(msg){ }
    virtual ~ThreadException()throw(){ }  
    virtual const char* what() const throw() {
        return (const char*)"ThreadException";
    }
};

class APIException: public TCEException
{
public:
    APIException(string msg): TCEException(msg){ }
    virtual ~APIException()throw(){ }  
    virtual const char* what() const throw() {
        return (const char*)"APIException";
    }
};

#endif
