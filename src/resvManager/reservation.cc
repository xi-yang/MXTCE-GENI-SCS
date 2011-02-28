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

#include "reservation.hh"


TDelta* TLinkDelta::Clone()
{
    TSchedule* sched = (this->schedule ? this->schedule->Clone():NULL);
    TDelta* td = (TDelta*)new TLinkDelta(this->resvName, sched, this->targetResource, this->bandwidth);
}


void TLinkDelta::Apply()
{
    if (targetResource == NULL)
        return;
    Link* link = (Link*)targetResource;
    for (int i = 0; i < 8; i++) {
        link->GetUnreservedBandwidth()[i] -= this->bandwidth;
        if (link->GetUnreservedBandwidth()[i] < 0) 
            link->GetUnreservedBandwidth()[i] = 0;
    }
}


void TLinkDelta::Revoke()
{
    if (targetResource == NULL)
        return;
    Link* link = (Link*)targetResource;
    for (int i = 0; i < 8; i++) {
        link->GetUnreservedBandwidth()[i] += this->bandwidth;
        if (link->GetUnreservedBandwidth()[i] >  link->GetMaxReservableBandwidth()) 
            link->GetUnreservedBandwidth()[i] = link->GetMaxReservableBandwidth();
    } 
}


list<TDelta*> TReservation::CloneDeltas()
{
    list<TDelta*> deltaList;
    list<TDelta*>::iterator itd = this->deltaCache.begin();
    for (; itd != this->deltaCache.end(); itd++)
        deltaList.push_back((*itd)->Clone());
    return deltaList;
}

// The serviceTopology may contain abstract links. We may need to wait for the reservation beeing full computed 
// and serviceTopology being completele updated before we call this method.
void TReservation::BuildDeltaCache()
{
    //clean up list ???
    list<TDelta*>::iterator itd = this->deltaCache.begin();
    for (; itd != this->deltaCache.end(); itd++)
    {
        delete (*itd);
        itd = this->deltaCache.erase(itd);
    }
    //rebuild
    list<TLink*>& links = serviceTopology->GetLinks();
    list<TLink*>::iterator itl = links.begin();
    TDelta* delta;
    for (; itl != links.end(); itl++)
    {
        TLink* link = *itl;
        ISCD* iscd = link->GetTheISCD();
        switch(iscd->switchingType)
        {
        case LINK_IFSWCAP_PSC1:
        case LINK_IFSWCAP_PSC2:
        case LINK_IFSWCAP_PSC3:
        case LINK_IFSWCAP_PSC4:
            delta = new TLinkDelta_PSC(name, NULL, link, link->GetMaxReservableBandwidth());
            break;
        case LINK_IFSWCAP_L2SC:
            delta = new TLinkDelta_L2SC(name, NULL, link, link->GetMaxReservableBandwidth());
            //add vlans later
            break;
        case LINK_IFSWCAP_TDM:
            delta = new TLinkDelta_TDM(name, NULL, link, link->GetMaxReservableBandwidth());
            //add timeslots later
            break;
        case LINK_IFSWCAP_LSC:
            delta = new TLinkDelta_LSC(name, NULL, link, link->GetMaxReservableBandwidth());
            //add wavelengths later
            break;
        default:
            delta = NULL;
            ;
        }
        if (delta != NULL)
        {
            list<TSchedule*>::iterator its = schedules.begin();
            for (; its != schedules.end(); its++)
            {
                if (its != schedules.begin())
                    delta = delta->Clone();
                delta->SetSchedule((*its)->Clone());
                deltaCache.push_back(delta);
            }
        }
    }
}

list<TDelta*> TReservation::GetDeltasByResource(Resource* resource)
{
    list<TDelta*> deltaList;
    list<TDelta*>::iterator itd = this->deltaCache.begin();
    for (; itd != this->deltaCache.end(); itd++)
    {
        if ((*itd)->GetTargetResource() == resource)
            deltaList.push_back(*itd);
    }
    return deltaList;
}

list<TDelta*> TReservation::GetDeltasBySchedule(TSchedule* schedule)
{
    list<TDelta*> deltaList;
    list<TDelta*>::iterator itd = this->deltaCache.begin();
    for (; itd != this->deltaCache.end(); itd++)
    {
        if (*(*itd)->GetSchedule() == *schedule)
            deltaList.push_back(*itd);
    }
    return deltaList;
}

void TReservation::ApplyDeltas()
{
    list<TDelta*>::iterator itd = this->deltaCache.begin();
    for (; itd != this->deltaCache.end(); itd++)
    {
        (*itd)->Apply();
    }    
}

void TReservation::RevokeDeltas()
{
    list<TDelta*>::iterator itd = this->deltaCache.begin();
    for (; itd != this->deltaCache.end(); itd++)
    {
        (*itd)->Revoke();
    }    
}
