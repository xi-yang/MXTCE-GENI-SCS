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
    return td;
}


void TLinkDelta::Apply()
{
    if (targetResource == NULL)
        return;
    if (applied)
        return;
    gettimeofday (&appliedTime, NULL);
    applied = true;
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
    if (!applied)
        return;
    gettimeofday (&appliedTime, NULL);
    applied = false;
    Link* link = (Link*)targetResource;
    for (int i = 0; i < 8; i++) {
        link->GetUnreservedBandwidth()[i] += this->bandwidth;
        if (link->GetUnreservedBandwidth()[i] >  link->GetMaxReservableBandwidth()) 
            link->GetUnreservedBandwidth()[i] = link->GetMaxReservableBandwidth();
    } 
}


void TLinkDelta::Combine(TDelta* delta)
{
    bandwidth += ((TLinkDelta*)delta)->GetBandwidth();
}


void TLinkDelta::Decombine(TDelta* delta)
{
    bandwidth -= ((TLinkDelta*)delta)->GetBandwidth();
    if (bandwidth < 0) bandwidth = 0;
}

void TLinkDelta::Join(TDelta* delta)
{
    if (bandwidth < ((TLinkDelta*)delta)->GetBandwidth())
        bandwidth = ((TLinkDelta*)delta)->GetBandwidth();
}



/**
  * Assume the first ISCD represents the link layer. Will support multi-ISCDs in future.
  */
TDelta* TLinkDelta_PSC::Clone()
{
    TSchedule* sched = (this->schedule ? this->schedule->Clone():NULL);
    TDelta* td = (TDelta*)new TLinkDelta_PSC(this->resvName, sched, this->targetResource, this->bandwidth);
    return td;
}


void TLinkDelta_PSC::Apply()
{
    if (applied)
        return;
    TLinkDelta::Apply();
    TLink* link = (TLink*)targetResource;
    ISCD_PSC* iscd = (ISCD_PSC*)link->GetTheISCD();
    assert(iscd->switchingType >= LINK_IFSWCAP_PSC1 && iscd->switchingType <= LINK_IFSWCAP_PSC4);
}


void TLinkDelta_PSC::Revoke()
{
    if (!applied)
        return;
    TLinkDelta::Revoke();
    TLink* link = (TLink*)targetResource;
    ISCD_PSC* iscd = (ISCD_PSC*)link->GetTheISCD();
    assert(iscd->switchingType >= LINK_IFSWCAP_PSC1 && iscd->switchingType <= LINK_IFSWCAP_PSC4);
}


TDelta* TLinkDelta_L2SC::Clone()
{
    TSchedule* sched = (this->schedule ? this->schedule->Clone():NULL);
    TDelta* td = (TDelta*)new TLinkDelta_L2SC(this->resvName, sched, this->targetResource, this->bandwidth, this->vlanTags);
    return td;
}


void TLinkDelta_L2SC::Apply()
{
    if (targetResource == NULL)
        return;
    if (applied)
        return;
    TLinkDelta::Apply();
    TLink* link = (TLink*)targetResource;
    ISCD_L2SC* iscd = (ISCD_L2SC*)link->GetTheISCD();
    assert(iscd->switchingType == LINK_IFSWCAP_L2SC);
    iscd->availableVlanTags.DeleteTags(this->vlanTags.TagBitmask(), MAX_VLAN_NUM);
    iscd->assignedVlanTags.Join(this->vlanTags);
}


void TLinkDelta_L2SC::Revoke()
{
    if (targetResource == NULL)
        return;
    if (!applied)
        return;
    TLinkDelta::Revoke();
    TLink* link = (TLink*)targetResource;
    ISCD_L2SC* iscd = (ISCD_L2SC*)link->GetTheISCD();
    assert(iscd->switchingType == LINK_IFSWCAP_L2SC);
    iscd->availableVlanTags.Join(this->vlanTags);
    iscd->assignedVlanTags.DeleteTags(this->vlanTags.TagBitmask(), MAX_VLAN_NUM);
}



void TLinkDelta_L2SC::Combine(TDelta* delta)
{
    TLinkDelta::Combine(delta);
    vlanTags.Join(((TLinkDelta_L2SC*)delta)->GetVlanTags());
}


void TLinkDelta_L2SC::Decombine(TDelta* delta)
{
    TLinkDelta::Decombine(delta);
    vlanTags.DeleteTags(((TLinkDelta_L2SC*)delta)->GetVlanTags().TagBitmask(), MAX_VLAN_NUM);
}

void TLinkDelta_L2SC::Join(TDelta* delta)
{
    TLinkDelta::Join(delta);
    vlanTags.Join(((TLinkDelta_L2SC*)delta)->GetVlanTags());
}



TDelta* TLinkDelta_TDM::Clone()
{
    TSchedule* sched = (this->schedule ? this->schedule->Clone():NULL);
    TDelta* td = (TDelta*)new TLinkDelta_TDM(this->resvName, sched, this->targetResource, this->bandwidth, this->timeslots);
    return td;
}


void TLinkDelta_TDM::Apply()
{
    if (targetResource == NULL)
        return;
    if (applied)
        return;
    // TODO: round-up bandwidth
    TLinkDelta::Apply();
    TLink* link = (TLink*)targetResource;
    ISCD_TDM* iscd = (ISCD_TDM*)link->GetTheISCD();
    assert(iscd->switchingType == LINK_IFSWCAP_TDM);
    iscd->availableTimeSlots.DeleteTags(this->timeslots.TagBitmask(), MAX_TIMESLOTS_NUM);
    iscd->assignedTimeSlots.Join(this->timeslots);
}


void TLinkDelta_TDM::Revoke()
{
    if (targetResource == NULL)
        return;
    if (!applied)
        return;
    // TODO: round-up bandwidth
    TLinkDelta::Revoke();
    TLink* link = (TLink*)targetResource;
    ISCD_TDM* iscd = (ISCD_TDM*)link->GetTheISCD();
    assert(iscd->switchingType == LINK_IFSWCAP_TDM);
    iscd->availableTimeSlots.Join(this->timeslots);
    iscd->assignedTimeSlots.DeleteTags(this->timeslots.TagBitmask(), MAX_TIMESLOTS_NUM);
}



TDelta* TLinkDelta_LSC::Clone()
{
    TSchedule* sched = (this->schedule ? this->schedule->Clone():NULL);
    TDelta* td = (TDelta*)new TLinkDelta_LSC(this->resvName, sched, this->targetResource, this->bandwidth, this->wavelengths);
    return td;
}


void TLinkDelta_LSC::Apply()
{
    if (targetResource == NULL)
        return;
    if (applied)
        return;
    // TODO: round-up bandwidth
    TLinkDelta::Apply();
    TLink* link = (TLink*)targetResource;
    ISCD_LSC* iscd = (ISCD_LSC*)link->GetTheISCD();
    assert(iscd->switchingType == LINK_IFSWCAP_LSC);
    iscd->availableWavelengths.DeleteTags(this->wavelengths.TagBitmask(), MAX_WAVE_NUM);
    iscd->assignedWavelengths.Join(this->wavelengths);
}


void TLinkDelta_LSC::Revoke()
{
    if (targetResource == NULL)
        return;
    if (!applied)
        return;
    // TODO: round-up bandwidth
    TLinkDelta::Revoke();
    TLink* link = (TLink*)targetResource;
    ISCD_LSC* iscd = (ISCD_LSC*)link->GetTheISCD();
    assert(iscd->switchingType == LINK_IFSWCAP_LSC);
    iscd->availableWavelengths.Join(this->wavelengths);
    iscd->assignedWavelengths.DeleteTags(this->wavelengths.TagBitmask(), MAX_WAVE_NUM);
}


list<TDelta*> TReservation::CloneDeltas()
{
    list<TDelta*> deltaList;
    list<TDelta*>::iterator itd = this->deltaCache.begin();
    for (; itd != this->deltaCache.end(); itd++)
        deltaList.push_back((*itd)->Clone());
    return deltaList;
}

/**
  * The serviceTopology may contain abstract links. We may need to wait for the reservation beeing
  *  fully computed and serviceTopology being completele updated before we call this method.
  */
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
            // use maxResvableBw of the serviceTopology as the delta bandwidth.
            delta = new TLinkDelta_PSC(name, NULL, link, link->GetMaxReservableBandwidth());
            break;
        case LINK_IFSWCAP_L2SC:
            delta = new TLinkDelta_L2SC(name, NULL, link, link->GetMaxReservableBandwidth());
            // add vlans from link ISCD (may be changed in later computation)
            ((TLinkDelta_L2SC*)delta)->GetVlanTags().AddTags(((ISCD_L2SC*)iscd)->assignedVlanTags.TagBitmask(), MAX_VLAN_NUM);
            break;
        case LINK_IFSWCAP_TDM:
            delta = new TLinkDelta_TDM(name, NULL, link, link->GetMaxReservableBandwidth(), MAX_TIMESLOTS_NUM);
            // TODO: add timeslots to delta
            break;
        case LINK_IFSWCAP_LSC:
            delta = new TLinkDelta_LSC(name, NULL, link, link->GetMaxReservableBandwidth(), MAX_WAVE_NUM);
            // TODO: add wavelengths to delta
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

