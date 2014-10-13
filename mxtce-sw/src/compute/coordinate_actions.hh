/*
 * Copyright (c) 2013-2014
 * GENI Project.
 * University of Maryland /Mid-Atlantic Crossroads (UMD/MAX).
 * All rights reserved.
 *
 * Created by Xi Yang 2014
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

#ifndef COORDINATE_ACTIONS_HH
#define	COORDINATE_ACTIONS_HH


#include "action.hh"
#include "tewg.hh"

class Action_ProcessRequestTopology_Coordinate: public Action
{
    protected:
    
    public:
        Action_ProcessRequestTopology_Coordinate(): Action(){ }
        Action_ProcessRequestTopology_Coordinate(string& n, ComputeWorker* w): Action(n, w) { }
        virtual ~Action_ProcessRequestTopology_Coordinate() { }
    
        virtual void Process();
        virtual bool ProcessChildren();
        virtual bool ProcessMessages();
        virtual void Finish();
        virtual void CleanUp();
};


class Action_CheckResult_Coordinate: public Action
{
    protected:
    
    public:
        Action_CheckResult_Coordinate(): Action(){ }
        Action_CheckResult_Coordinate(string& cn, string& n, ComputeWorker* w): Action(n, w) { }
        virtual ~Action_CheckResult_Coordinate() { }
    
        virtual void Process();
        virtual bool ProcessChildren();
        virtual bool ProcessMessages();
        virtual void Finish();
        virtual void CleanUp();
};


class Action_ProcessSubworker_Coordinate: public Action
{
    protected:
        list<Apimsg_user_constraint*>* = _userConstraintList;
        list<ComputeResult*>* = _computeResultList;
        void _Init();

    public:
        Action_ProcessSubworker_Coordinate(): Action(){ _Init(); }
        Action_ProcessSubworker_Coordinate(string& cn, string& n, ComputeWorker* w): Action(n, w) { _Init(); }
        virtual ~Action_ProcessSubworker_Coordinate() { }
    
        virtual void Process();
        virtual bool ProcessChildren();
        virtual bool ProcessMessages();
        virtual void Finish();
        virtual void CleanUp();
        virtual void* GetData(string& dataName);
};


#endif
