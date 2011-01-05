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
 
#ifndef __EVENT_HH__
#define __EVENT_HH__
#include <list>
#include <map>
#include "types.hh"

using namespace std;

////////////Event Objects/////////////
#define FOREVER  (-1)

class EventMaster;

typedef enum 
{
    EVENT_PRIORITY=0,
    EVENT_TIMER,
    EVENT_READ,
    EVENT_WRITE,
    EVENT_READY,
    EVENT_IDLE
} EventType;

typedef  int (*CallBack)(void*, void*);

class Event
{
private:
     bool shouldAutoDelete;
     bool isObsolete;
protected:
     EventType type;
     CallBack callFunc;
     void *param1;
     void *param2;
     int repeats;
public:
    Event(): type(EVENT_PRIORITY), repeats(0), callFunc (NULL), param1(NULL), param2(NULL), shouldAutoDelete(false), isObsolete(false) {}
    Event(int repeats_val): type(EVENT_PRIORITY), repeats(repeats_val), callFunc (NULL), param1(NULL), param2(NULL), shouldAutoDelete(false), isObsolete(false) { }
    Event(EventType eType, int repeats_val): type(eType), repeats(repeats_val), callFunc (NULL), param1(NULL), param2(NULL), shouldAutoDelete(false), isObsolete(false) { }
    virtual ~Event(){}
    EventType Type() {return type;}
    void SetType(EventType eType) {type = eType;}
    void SetCallBack(CallBack call, void* call_param1=NULL, void* call_param2=NULL) 
        { callFunc = call; param1 = call_param1; param2 = call_param2; }
    int Repeats() {return repeats;}
    void SetRepeats(int repeats_val) {repeats = repeats_val;}
    virtual void Run() { if (callFunc) callFunc(param1, param2); }
    virtual void Cycle() {assert (repeats>0 || repeats == FOREVER); if (repeats > 0) repeats --;}
    bool ShouldAutoDelete() { return shouldAutoDelete; }
    void SetAutoDelete(bool blAutoDelete) { shouldAutoDelete = blAutoDelete; }

    bool Obsolete() { return isObsolete; }
    void SetObsolete(bool blObsolete) { isObsolete = blObsolete; }
   
    friend class EventMaster;
};

////////////struct timeval operators/////////////
extern inline struct timeval operator-(struct timeval& a, struct timeval& b)
{   
    int sec_diff = a.tv_sec - b.tv_sec;
    int usec_diff = a.tv_usec - b.tv_usec;
    if (sec_diff >= 0 && usec_diff >= 0 || sec_diff < 0 && usec_diff <= 0)
        return (struct timeval){sec_diff, usec_diff};
    else if (sec_diff > 0 && usec_diff < 0)
        return (struct timeval){--sec_diff, 1000000L + usec_diff};
    else if (sec_diff < 0 && usec_diff > 0)
        return (struct timeval){++sec_diff, 1000000L - usec_diff};
    //never reached    
    return (struct timeval){0, 0};
}

extern inline struct timeval operator+(struct timeval& a, struct timeval& b)
{
    int sec_diff = a.tv_sec + b.tv_sec;
    int usec_diff = a.tv_usec + b.tv_usec;
    assert(sec_diff >=0 && usec_diff >= 0);
    if (usec_diff >= 1000000L)
    {
        sec_diff ++;
        usec_diff -= 1000000L;
    }
    return (struct timeval){sec_diff, usec_diff};
}


extern inline bool operator<(struct timeval& a, struct timeval& b)
{
    return (a.tv_sec < b.tv_sec ||a.tv_sec == b.tv_sec && a.tv_usec < b.tv_usec); 
}

extern inline bool operator==(struct timeval& a, struct timeval& b)
{
    return (a.tv_sec == b.tv_sec  && a.tv_usec == b.tv_usec); 
}
/////////////////////////////////////////////
class Timer: public Event
{
private:
    struct timeval due;
    struct timeval interval;
public:
    Timer(struct timeval due_val) : due(due_val),  interval ((struct timeval){0, 0}), Event(EVENT_TIMER, 0) {}
    Timer(struct timeval due_val, struct timeval interval_val, int repeats_val) 
        : due(due_val), interval(interval_val), Event(EVENT_TIMER, repeats_val) {}
    Timer(int sec, int usec) :  interval ((struct timeval){sec, usec}), Event(EVENT_TIMER, 0) 
        {   struct timeval now;
             gettimeofday (&now, NULL);
             due = now + interval;    }    
    Timer(int sec, int usec, int repeats_val) : interval((struct timeval){sec, usec}), Event(EVENT_TIMER, repeats_val) 
        {   struct timeval now;
             gettimeofday (&now, NULL);
             due = now + interval;    }
    virtual ~Timer() {}

    void SetInterval(int sec, int usec) 
        {  struct timeval interval, now; gettimeofday (&now, NULL); 
            interval.tv_sec = sec; interval.tv_usec = usec; due = now + interval;   }
    void Restart()
        {   struct timeval now;
             gettimeofday (&now, NULL);
             due = now + interval;    }
    inline bool operator<(Timer& otherTimer)
        {   return (this->due < otherTimer.due); }
    inline bool operator==(Timer& otherTimer)
        {   return (this->due == otherTimer.due); }
    virtual void Run() { Event::Run(); }
    virtual void Cycle() 
        {  assert (repeats>0 || repeats == FOREVER);  if (repeats > 0) repeats --;  due = due + interval; }

    friend class EventMaster;
};


class Selector: public Event
{
protected:
    int fd;
public:
    Selector(int fd_val, EventType eType, int repeats_val) :fd(fd_val), Event(eType, repeats_val) {}
    virtual ~Selector(){}
    virtual void Run() { Event::Run(); }
    virtual void Close();
    int Socket() { return fd; };
    void SetSocket(int x) { fd = x; }
    friend class EventMaster;
};

class Reader: public Selector
{
public:
    Reader(int fd_val):Selector(fd_val, EVENT_READ, 0){}
    virtual ~Reader(){}
    virtual void Run() { Event::Run(); }
    friend class EventMaster;
};

class Writer: public Selector
{
public:
    Writer(int fd_val):Selector(fd_val, EVENT_WRITE, 0){}
    virtual ~Writer(){}
    virtual void Run() { Event::Run(); }
    friend class EventMaster;
};


////////////EventMaster  (A singleton class!)/////////////

class EventMaster
{
private:
    static map<unsigned long, EventMaster*> threadMasters;
    fd_set readfd;
    fd_set writefd;
    fd_set exceptfd;
    
    list<Event*> eventLists[5];

    #define priorities eventLists[0]
    #define timers eventLists[1]
    #define reads eventLists[2]
    #define writes eventLists[3]
    #define ready eventLists[4]
    
    EventMaster(){ FD_ZERO(&readfd); FD_ZERO(&writefd); FD_ZERO(&exceptfd); }
public:
    ~EventMaster();
    static EventMaster* GetInstance();
    static EventMaster* GetProcessInstance();
    static EventMaster* GetThreadInstance(unsigned long tid);
    list<Event*>& GetEventList(EventType type) { return eventLists[(int)type]; }
    void Schedule(Event *event);
    void Remove(Event *event);
    struct timeval WaitLimit ();
    void ModifyFDSets (fd_set *pReadfd, fd_set *pWritefd);
    Event* Fetch();
    void Run();
   
    friend class Event;
};

extern EventMaster* coreEventMaster;


#endif
