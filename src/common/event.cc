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
#include "event.hh"
#include <errno.h>

EventMaster::~EventMaster()
{
    for (int i = 0; i < 5; i++)
    {
        list<Event*>::iterator it;
        for (it = eventLists[i].begin(); it != eventLists[i].end(); it++)
        {
            Event * e = *it;
            if (e && e->ShouldAutoDelete() == true)
                delete e;
        }
        eventLists[i].clear();
    }
}

void EventMaster::Schedule(Event* event)
{
    list<Event*>::iterator iter;

    switch(event->type)
    {
    case EVENT_TIMER:
        Timer *timer;
        
        for (iter = timers.begin(); iter != timers.end(); iter++)
        {
            timer = (Timer *)*iter;
            if (timer && (*(Timer*)event) < *timer)
            {
                timers.insert(iter, event);
                return;
            }
        }
        timers.push_back(event);
        break;
        
    case EVENT_PRIORITY:
        priorities.push_back(event);
        break;
    case EVENT_NICE:
        nices.push_back(event);
        break;
    case EVENT_READ:
       if (((Selector*)event)->fd < 0)
       {
            event->SetRepeats(0);
            event->SetObsolete(true);
            return;
       }
       if (FD_ISSET(((Selector*)event)->fd, &readfd))
       {
            LOGF("There has been read on [%d]\n", ((Selector*)event)->fd);
            return;
        }
        FD_SET(((Selector*)event)->fd, &readfd);
        reads.push_back(event);
        break;
    case EVENT_WRITE:
       if (((Selector*)event)->fd < 0)
       {
            event->SetRepeats(0);
            event->SetObsolete(true);
            return;
       }
       if (FD_ISSET(((Selector*)event)->fd, &writefd))
       {
            LOGF("There has been write on [%d]\n", ((Selector*)event)->fd);
            return;
        }
        FD_SET(((Selector*)event)->fd, &writefd);
        writes.push_back(event);
        break;
    default:
        LOGF("No such an event type (%d)", event->type);
    }
}

void
EventMaster::Remove(Event *event)
{
    if (event == NULL || event->type > 5 || event->type < 0)
        return;
    
    list<Event*> *eList = &eventLists[event->type];
    event->SetObsolete(true);
    event->SetRepeats(0);

    if (event->type == EVENT_READ && ((Selector*)event)->fd > 0 && (FD_ISSET (((Selector*)event)->fd, &readfd)))
    {
        FD_CLR(((Selector*)event)->fd, &readfd);
        close(((Selector*)event)->fd);
        ((Selector*)event)->fd = -1;	
    }
    if (event->type == EVENT_WRITE && ((Selector*)event)->fd > 0 && (FD_ISSET (((Selector*)event)->fd, &writefd)))
    {
        FD_CLR(((Selector*)event)->fd, &writefd);
        close(((Selector*)event)->fd);
        ((Selector*)event)->fd = -1;
    }

    list<Event*>::iterator ite;
    for (ite = eList->begin(); ite != eList->end(); ite++)
        if ((*ite) == event)
            ite = eList->erase(ite);
}

void
EventMaster::Run()
{
    Event *event;
    stopped = false;
    while (!stopped)
    {
        event = this->Fetch();

        if (!event)
            continue;

        event->Run();

        if (event->repeats >  0 || event->repeats == FOREVER)
        {
            event->Cycle();
            this->Schedule(event);
        } else if (event->repeats == 0)
        {
            event->SetObsolete(true);
        }

        if (event->Obsolete() && event->ShouldAutoDelete())
        {
            if (event->type == EVENT_READ && ((Selector*)event)->fd > 0)
            {
                if (FD_ISSET (((Selector*)event)->fd, &readfd))
                    FD_CLR(((Selector*)event)->fd, &readfd);
                close(((Selector*)event)->fd);
                ((Selector*)event)->fd = -1;
            }
            if (event->type == EVENT_WRITE && ((Selector*)event)->fd > 0)
            {
                if (FD_ISSET (((Selector*)event)->fd, &writefd))
                    FD_CLR(((Selector*)event)->fd, &writefd);
                close(((Selector*)event)->fd);
                ((Selector*)event)->fd = -1;
            }
            delete event;
        }
    }
}


void EventMaster::Stop()
{
    stopped = true;
}


struct timeval
EventMaster::WaitLimit ()
{
  struct timeval timer_now, timer_min;

  gettimeofday (&timer_now, NULL);

  list<Event*>::iterator iter;
  Timer* timer = NULL;

  for (iter = timers.begin(); iter != timers.end(); iter++)
  {
      if (! timer)
          timer = (Timer *)*iter;
      else if (*(Timer*)(*iter) < *timer)
          *timer = *(Timer *)*iter;
  }

  if (timer && timers.size() > 0)
    {
      timer_min = timer->due;
      timer_min = timer_min - timer_now;
      if (timer_min.tv_sec < 0)
	{
	  timer_min.tv_sec = 0;
	  timer_min.tv_usec = 50;
	}
      return timer_min;
    }

  timer_min = (struct timeval){0, 50};
  return timer_min;
}

void
EventMaster::ModifyFDSets (fd_set *pReadfd, fd_set *pWritefd)
{
  Selector *event;
  list<Event*>::iterator iter = reads.begin();

  while (iter != reads.end())
    {
        event = (Selector*)*iter;
        if (!event)
            break;
        if (FD_ISSET (event->fd, pReadfd))
        {
            if (FD_ISSET (event->fd, &readfd))
                FD_CLR(event->fd, &readfd);
            iter = reads.erase(iter);
            ready.push_back (event);
			continue;
        }
		iter++;
    }

  iter = writes.begin();
  while (iter != writes.end())
    {
        event = (Selector*)*iter;              
        if (!event)
            break;
        if (FD_ISSET (event->fd, pWritefd))
  		 {
            if (FD_ISSET (event->fd, &writefd))
                FD_CLR(event->fd, &writefd);
            iter = writes.erase (iter);
            ready.push_back (event);
			continue;
	  	 }
		iter++;
    }
}

Event* 
EventMaster::Fetch()
{
  Event* event;
  Timer* timer;
  
  fd_set s_readfd;
  fd_set s_writefd;
  fd_set s_exceptfd;
  
  struct timeval timer_now;
  struct timeval timeout;

  list<Event*>::iterator iter;
  int numSelect;

  while (!stopped)
  {
      // Handling events with priorities
      if ((!priorities.empty()) && (event = priorities.front()))
      {
          priorities.pop_front();
          if (event->Nice())
          {
              event->SetType(EVENT_NICE);
              nices.push_back(event);
          }
          else
              return event;
      }

      // Handling timers
      gettimeofday (&timer_now, NULL);

      for (iter = timers.begin(); iter != timers.end(); iter++)
      {
          timer = (Timer*)*iter;
          if (timer->due < timer_now || timer->due == timer_now)
          {
              timers.remove (timer);
              return timer;
          }
      }
      
      // Handling ready events
      if ((!ready.empty()) && (event = ready.front()))
      {
          ready.pop_front();
          return event;
      }

      //Calculating timeout form select operation
      timeout = WaitLimit ();

      s_readfd = readfd;
      s_writefd = writefd;
      s_exceptfd = exceptfd;
      numSelect = select (FD_SETSIZE, &s_readfd, &s_writefd, &s_exceptfd, &timeout);

      if (numSelect == 0)
        goto _nice;

      if (numSelect < 0)
      {
        // A signal was delivered before the time limit expired.
        if (errno == EINTR)
            goto _nice;
        //Other error  //LOG
        return NULL;
      }

      ModifyFDSets (&s_readfd, &s_writefd);

      if ((!ready.empty()) && (event = ready.front()))
      {
          ready.pop_front();
          return event;
      }

    _nice:
      // Handling nice events
      if ((!nices.empty()) && (event = nices.front()))
      {
          nices.pop_front();
          return event;
      }
  }

  return NULL;
}

void Selector::Close()
{
    if (fd < 0)
        return;

    close(fd);
    //@@@@ eventMaster.Remove(this);
    fd = -1;
}

