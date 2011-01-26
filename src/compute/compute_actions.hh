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
 
#ifndef __COMPUTE_ACTIONS_HH__
#define __COMPUTE_ACTIONS_HH__

#include "action.hh"


class Action_ProcessRequestTopology: public Action
{
    protected:
    
    public:
        Action_ProcessRequestTopology(): Action(){ }
        Action_ProcessRequestTopology(ComputeWorker* w): Action(w) { }
        virtual ~Action_ProcessRequestTopology() { }
    
        virtual void Process();
        virtual bool ProcessChildren();
        virtual bool ProcessMessages();
        virtual void Finish();
        virtual void CleanUp();
};


class Action_CreateTEWG: public Action
{
protected:

public:
    Action_CreateTEWG(): Action(){ }
    Action_CreateTEWG(ComputeWorker* w): Action(w) { }
    virtual ~Action_CreateTEWG() { }

    //virtual void Run();
    //virtual void Wait();
    virtual void Process();
    virtual bool ProcessChildren();
    virtual bool ProcessMessages();
    virtual void Finish();
    virtual void CleanUp();
};


class Action_ComputeKSP: public Action
{
    protected:
    
    public:
        Action_ComputeKSP(): Action(){ }
        Action_ComputeKSP(ComputeWorker* w): Action(w) { }
        virtual ~Action_ComputeKSP() { }
    
        virtual void Process();
        virtual bool ProcessChildren();
        virtual bool ProcessMessages();
        virtual void Finish();
        virtual void CleanUp();
};


class Action_FinalizeServiceTopology: public Action
{
    protected:
    
    public:
        Action_FinalizeServiceTopology(): Action(){ }
        Action_FinalizeServiceTopology(ComputeWorker* w): Action(w) { }
        virtual ~Action_FinalizeServiceTopology() { }
    
        virtual void Process();
        virtual bool ProcessChildren();
        virtual bool ProcessMessages();
        virtual void Finish();
        virtual void CleanUp();
};

#endif
