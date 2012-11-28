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

#include "exception.hh"
#include "log.hh"
#include "tewg.hh"
#include "reservation.hh"
#include "scheduling.hh"
#include <algorithm> 

TDomain* TDomain::Clone(bool newSubLevels)
{
    TDomain* td = new TDomain(this->_id, this->name, this->address);
    td->disabled = this->disabled;
    map<string, Node*, strcmpless>::iterator itn = this->nodes.begin();
    for (; itn != this->nodes.end(); itn++)
        td->nodes[(*itn).first] = (newSubLevels ? ((TNode*)(*itn).second)->Clone(newSubLevels) : (*itn).second);
    return td;            
}


void TNode::AddLocalLink(TLink* link)
{
    if (HasLocalLink(link))
    {
        char buf[128];
        snprintf(buf, 128, "Node::AddLocalLink raises Excaption: local link %s:%s has already existed.", 
            link->GetPort()->GetName().c_str(), link->GetName().c_str());
        throw TEDBException(buf);
    }
    this->lclLinks.push_back(link);
}


void TNode::AddRemoteLink(TLink* link)
{
    if (!link->VerifyRemoteLink())
        throw TEDBException((char*)"TNode::AddRmoteLink raises Excaption: invalid remote link parenet port is 'null'.");
    else if (HasRemoteLink(link))
    {
        char buf[128];
        snprintf(buf, 128, "Node::AddRemoteLink raises Excaption: remote link %s:%s has already existed.", 
            link->GetPort()->GetName().c_str(), link->GetName().c_str());
        throw TEDBException(buf);
    }
    this->rmtLinks.push_back(link);
}


bool TNode::HasLocalLink(TLink* link)
{
    list<TLink*>::iterator itl;
    for (itl = lclLinks.begin(); itl != lclLinks.end(); itl++)
        if ( (*itl) == link)
            return true;
    return false;
}


bool TNode::HasRemoteLink(TLink* link)
{
    list<TLink*>::iterator itl;
    for (itl = rmtLinks.begin(); itl != rmtLinks.end(); itl++)
        if ( (*itl) == link)
            return true;
    return false;
}

TNode* TNode::Clone(bool newSubLevels)
{
    TNode* tn = new TNode(this->_id, this->name, this->address);
    tn->domain = this->domain;
    tn->disabled = this->disabled;
    tn->visited = this->visited;
    map<string, Port*, strcmpless>::iterator itp = this->ports.begin();
    for (; itp != this->ports.end(); itp++)
        tn->ports[(*itp).first] = (newSubLevels ? ((TPort*)(*itp).second)->Clone(newSubLevels) : (*itp).second);
    list<TLink*>::iterator itl = this->lclLinks.begin();
    for (; itl != this->lclLinks.end(); itl++)
    {
        TLink* tl = (*itl);
        if (tl->GetPort() == NULL && newSubLevels)
            tl = tl->Clone();
        tn->lclLinks.push_back(tl);
    }
    itl = this->rmtLinks.begin();
    for (; itl != this->rmtLinks.end(); itl++)
    {
        tn->rmtLinks.push_back(*itl);
    }
    return tn;
}


TPort* TPort::Clone(bool newSubLevels)
{
    TPort* tp = new TPort(this->_id, this->name, this->address);
    tp->node = this->node;
    tp->disabled = this->disabled;
    tp->visited = this->visited;
    tp->maxBandwidth = this->maxBandwidth;
    tp->maxReservableBandwidth = this->maxReservableBandwidth;
    tp->minReservableBandwidth = this->minReservableBandwidth;
    tp->bandwidthGranularity = this->minReservableBandwidth;
    for (int i = 0; i < 8; i++)
        tp->unreservedBandwidth[i] = this->unreservedBandwidth[i];
    map<string, Link*, strcmpless>::iterator itl = this->links.begin();
    for (; itl != this->links.end(); itl++)
        tp->links[(*itl).first] = (newSubLevels ? ((TLink*)(*itl).second)->Clone() : (*itl).second);
    // TODO: Clone TPoint when implemented
    return tp;
}


bool TLink::VerifyEdgeLink() 
{
    if (!edgeOnly || rmtEnd != NULL)
        return false;
    return (lclEnd != NULL && lclEnd->HasLocalLink(this));
    
}


bool TLink::VerifyRemoteLink() 
{
    return (remoteLink != NULL && remoteLink->GetRemoteLink() == this);
}


bool TLink::VerifyFullLink() 
{
    return (VerifyRemoteLink() && lclEnd->HasLocalLink(this) && lclEnd->HasRemoteLink((TLink*)remoteLink) && rmtEnd->HasLocalLink((TLink*)remoteLink) && rmtEnd->HasRemoteLink(this));
}

bool TLink::VerifyContainUrn(string& urn) 
{
    string domainName = "", nodeName = "", portName = "", linkName = "";
    ParseFQUrnShort(urn, domainName, nodeName, portName, linkName);
    if (domainName != this->GetPort()->GetNode()->GetDomain()->GetName())
        return false;
    if (nodeName.empty())
        return true;
    if (nodeName != this->GetPort()->GetNode()->GetName())
        return false;
    if (portName.empty())
        return true;
    if (portName != this->GetPort()->GetName())
        return false;
    if (linkName.empty())
        return true;
    if (linkName != this->GetName())
        return false;
    return true;
}

// assume all TE links have at least one ISCD. In most cases there is only one ISCD (the).
ISCD* TLink::GetTheISCD()
{
    assert(swCapDescriptors.size() > 0);
    return swCapDescriptors.front();
}


TLink* TLink::Clone()
{
    TLink* tl = new TLink(this->_id, this->name, this->address);
    tl->disabled = this->disabled;
    tl->visited = this->visited;
    tl->port = this->port;
    tl->metric = this->metric;
    tl->maxBandwidth = this->maxBandwidth;
    tl->maxReservableBandwidth = this->maxReservableBandwidth;
    tl->minReservableBandwidth = this->minReservableBandwidth;
    tl->bandwidthGranularity = this->minReservableBandwidth;
    tl->remoteLink = this->remoteLink;
    if (this->bag != NULL)
        tl->bag = this->bag->Clone();
    for (int i = 0; i < 8; i++)
        tl->unreservedBandwidth[i] = this->unreservedBandwidth[i];
    list<ISCD*>::iterator its = swCapDescriptors.begin();
    for (; its != swCapDescriptors.end(); its++)
        tl->swCapDescriptors.push_back((*its)->Duplicate());
    list<IACD*>::iterator ita = adjCapDescriptors.begin();
    for (; ita != adjCapDescriptors.end(); ita++)
        tl->adjCapDescriptors.push_back(*ita);
    list<Link*>::iterator itl = this->containerLinks.begin();
    for (; itl != this->containerLinks.end(); itl++)
        tl->containerLinks.push_back(*itl);
    itl = this->componentLinks.begin();
    for (; itl != this->componentLinks.end(); itl++)
        tl->componentLinks.push_back(*itl);
    return tl;
}


bool TLink::IsAvailableForTspec(TSpec& tspec)
{
    TSpec tspec_link;
    ISCD * iscd;
    list<ISCD*>::iterator it;
    for (it = swCapDescriptors.begin(); it != swCapDescriptors.end(); it++)
    {
        iscd = *it;
        assert(iscd);
        tspec_link.Update(iscd->switchingType, iscd->encodingType, iscd->capacity);

        if (tspec == tspec_link)
            return true;

        // TODO: This is testing code for now. We need to further consider meaning of bandwidth parameters in LSC and FSC.
        //$$ A temporary matching (available) condition for LSC and FSC links
        if ( (tspec.SWtype == LINK_IFSWCAP_LSC || tspec.SWtype == LINK_IFSWCAP_FSC)
            && tspec_link.SWtype == tspec.SWtype && tspec_link.ENCtype == tspec.ENCtype )
                return true;

        if (tspec <= tspec_link)
        {
            if (tspec.SWtype == LINK_IFSWCAP_TDM)
            {
               //if (tspec.ENCtype == LINK_IFSWCAP_ENC_G709ODUK && !this->GetOTNXInterfaceISCD(LINK_IFSWCAP_SUBTLV_SWCAP_TDM))
               //    return true; // no need for timeslots checking as that will be done by ::ProceedByUpdatingTimeslots
               //if ((((ISCD_TDM*)iscd)->concatenationType == STS1 && tspec.Bandwidth % 50 == 0)
               //    || (((ISCD_TDM*)iscd)->concatenationType == STS3C && tspec.Bandwidth % 150 == 0))
                   return true;
            }
            else if (tspec.SWtype >= LINK_IFSWCAP_PSC1 && 
                    tspec.SWtype <=  LINK_IFSWCAP_PSC4 ||
                    tspec.SWtype == LINK_IFSWCAP_L2SC)
                return true;
            else // tspec.SWtype == LINK_IFSWCAP_SUBTLV_SWCAP_LSC || LINK_IFSWCAP_SUBTLV_SWCAP_FSC
                continue;
        }
    }

    return false;
}

bool TLink::VerifyEdgeLinkTSpec(TSpec& tspec)
{
    TSpec tspec_link;
    ISCD * iscd;
    list<ISCD*>::iterator it;
    for (it = this->GetSwCapDescriptors().begin(); it != this->GetSwCapDescriptors().end(); it++)
    {
        iscd = *it;
        assert(iscd);
        tspec_link.Update(iscd->switchingType, iscd->encodingType, iscd->capacity);
        if (tspec_link.ENCtype != tspec.ENCtype)
            continue;

        if (tspec == tspec_link)
            return true;

        if (tspec <= tspec_link)
        {
            if (tspec.SWtype == LINK_IFSWCAP_TDM)
            {
                //if ((((ISCD_TDM*)iscd)->concatenationType == STS1 && tspec.Bandwidth % 50 == 0)
                //    || (((ISCD_TDM*)iscd)->concatenationType == STS3C && tspec.Bandwidth % 150 == 0))
                    return true;
            }
            else if (tspec.SWtype >= LINK_IFSWCAP_PSC1 && tspec.SWtype <=  LINK_IFSWCAP_PSC4 || tspec.SWtype == LINK_IFSWCAP_L2SC)
                return true;
            else // tspec.SWtype == LINK_IFSWCAP_SUBTLV_SWCAP_LSC || LINK_IFSWCAP_SUBTLV_SWCAP_FSC
                continue;
        }
    }
    return false;
}

bool TLink::CanBeLastHopTrunk(TSpec& tspec)
{
    if (remoteLink == NULL)
        return false;

    return ((TLink*)remoteLink)->VerifyEdgeLinkTSpec(tspec);
}

void TLink::ExcludeAllocatedVtags(ConstraintTagSet &vtagset)
{
    list<ISCD*>::iterator it;

    for (it = swCapDescriptors.begin(); it != swCapDescriptors.end(); it++)
    {
        if ((*it)->switchingType != LINK_IFSWCAP_L2SC)
            continue;
        ISCD_L2SC* iscd = (ISCD_L2SC*)(*it);
        vtagset.DeleteTags(iscd->assignedVlanTags.TagBitmask(), MAX_VLAN_NUM);
    }
}


//$$$$ Only constraining the forward direction (not checking the reverse; assuming symetric L2SC link configurations)
// TODO: handle "untagged" vlans (VTAG_UNTAGGED bit-4096 (1-based))
void TLink::ProceedByUpdatingVtags(ConstraintTagSet &head_vtagset, ConstraintTagSet &next_vtagset, bool do_translation)
{
    next_vtagset.Clear();
    list<ISCD*>::iterator it;
    bool any_vlan_ok = head_vtagset.HasAnyTag();
    bool non_vlan_link = true;

    // Add VLAN tags available for this link.
    for (it = swCapDescriptors.begin(); it != swCapDescriptors.end(); it++)
    {
         // The non-L2SC layers are temoperaty here and yet to remove.
        if ((*it)->switchingType != LINK_IFSWCAP_L2SC || (*it)->encodingType != LINK_IFSWCAP_ENC_ETH)
            continue;
        ISCD_L2SC* iscd = (ISCD_L2SC*)(*it);
        if (iscd->assignedVlanTags.Size()+iscd->availableVlanTags.Size() > 0)
        {
            non_vlan_link = false;
            next_vtagset.AddTags(iscd->availableVlanTags.TagBitmask(), MAX_VLAN_NUM);
        }
        // Do translation only if do_translation==true and iscd->vlanTranslation==true
        do_translation = (do_translation && iscd->vlanTranslation);
        // there should be only one ISCD_L2SC 
        break;
    }

    // Exclude VLAN tags used by any links on the local- or remote-end nodes of the link.
    list<TLink*>::iterator it_link;
    if (lclEnd)
    {
        for (it_link = lclEnd->GetRemoteLinks().begin(); it_link != lclEnd->GetRemoteLinks().end(); it_link++)
        {
            (*it_link)->ExcludeAllocatedVtags(next_vtagset);
        }
        for (it_link = lclEnd->GetLocalLinks().begin(); it_link != lclEnd->GetLocalLinks().end(); it_link++)
        {
            if (*it_link != this)
                (*it_link)->ExcludeAllocatedVtags(next_vtagset);
        }
    }
    if (rmtEnd)
    {
        for (it_link = rmtEnd->GetRemoteLinks().begin(); it_link != rmtEnd->GetRemoteLinks().end(); it_link++)
        {
            if (*it_link != this)
                (*it_link)->ExcludeAllocatedVtags(next_vtagset);
        }
        for (it_link = rmtEnd->GetLocalLinks().begin(); it_link != rmtEnd->GetLocalLinks().end(); it_link++)
        {
            (*it_link)->ExcludeAllocatedVtags(next_vtagset);
        }
    }
        
    if (non_vlan_link)
        next_vtagset = head_vtagset;
    else if (do_translation)
        ; //next_vtag unchanged after vlan translation, or
    else if (!any_vlan_ok) // do intersect without vlan translation
        next_vtagset.Intersect(head_vtagset); 
}


//$$$$ only constrain the forward direction (not checking the reverse)
void TLink::ProceedByUpdatingWaves(ConstraintTagSet &head_waveset, ConstraintTagSet &next_waveset, bool do_conversion)
{
    next_waveset.Clear();
    list<ISCD*>::iterator it;
    bool any_wave_ok = head_waveset.HasAnyTag();

    // Add Wavelength tags available for this link.
    for (it = swCapDescriptors.begin(); it != swCapDescriptors.end(); it++)
    {
         // The non-L2SC layers are temoperaty here and yet to remove.
        if ((*it)->switchingType != LINK_IFSWCAP_LSC)
            continue;
        ISCD_LSC* iscd = (ISCD_LSC*)(*it);
        if (iscd->assignedWavelengths.Size()+iscd->availableWavelengths.Size() > 0)
        {
            next_waveset.AddTags(iscd->availableWavelengths.TagBitmask(), MAX_VLAN_NUM);
        }
        // Convert wavelength only if do_conversion==true and iscd->wavelengthConversion==true
        do_conversion = (do_conversion && iscd->wavelengthConversion);
        // there should be only one ISCD_LSC 
        break;
    }

    // TODO: vendor specific wavelength constraint handling

     if (do_conversion)
        ; //next_waveset unchanged after wavelength conversion, or
     else if (!any_wave_ok)
        next_waveset.Intersect(head_waveset);
}


//$$$$ only constrain the forward direction (not checking the reverse)
void TLink::ProceedByUpdatingTimeslots(ConstraintTagSet &head_timeslotset, ConstraintTagSet &next_timeslotset)
{

    next_timeslotset.Clear();
    bool any_timeslot_ok = head_timeslotset.HasAnyTag();

    // TODO: vendor specific wavelength constraint handling
    //load up next_timeslotset from link

    if (!any_timeslot_ok)
        next_timeslotset.Intersect(head_timeslotset);
}


bool TLink::CrossingRegionBoundary(TSpec& tspec, TLink* next_link)
{
    // link may have multiple ISCDs. But as long as both this and next links can accomodarte the spec, no crossing is required on this link.
    if (next_link)
    {
        bool compatibleTspec = false;
        list<ISCD*>::iterator it_iscd;
        for (it_iscd = this->swCapDescriptors.begin(); it_iscd != this->swCapDescriptors.end(); it_iscd++)
        {
            if ((*it_iscd)->switchingType == tspec.SWtype && (*it_iscd)->encodingType == tspec.ENCtype)
            {
                compatibleTspec = true;
                break;
            }
        }
        if (compatibleTspec)
        {
            compatibleTspec = false;
            for (it_iscd = next_link->GetSwCapDescriptors().begin(); it_iscd != next_link->GetSwCapDescriptors().end(); it_iscd++)
                if ((*it_iscd)->switchingType == tspec.SWtype && (*it_iscd)->encodingType == tspec.ENCtype)
                    compatibleTspec = true;
            if (compatibleTspec)
                return false;
        }
    }

    // Check adaptation defined by IACD(s)
    list<IACD*>::iterator it_iacd;
    for (it_iacd = adjCapDescriptors.begin(); it_iacd != adjCapDescriptors.end(); it_iacd++)
    {
        //crossing from lower layer to upper layer
        if ((*it_iacd)->lowerLayerSwitchingType == tspec.SWtype && (*it_iacd)->lowerLayerEncodingType == tspec.ENCtype)
        {
            return true;
        }
        //crossing from upper layer to lower layer
        if ((*it_iacd)->upperLayerSwitchingType == tspec.SWtype && (*it_iacd)->upperLayerEncodingType == tspec.ENCtype)
            return true;

        // TODO: bandwidth adaptation criteria to be considered in the future.
    }

    // Check implicit adaptation
    if (this->remoteLink)
    {
        if (this->swCapDescriptors.size()*remoteLink->GetSwCapDescriptors().size() > 1) 
            return true; // at least one direction of the link supports multiple ISCDs
        if (this->swCapDescriptors.size() == 1 && remoteLink->GetSwCapDescriptors().size() == 1
            && this->GetTheISCD()->switchingType != ((TLink*)remoteLink)->GetTheISCD()->switchingType)
            return true; // one-to-one implicit adaptation
    }

    return false;
}

bool TLink::GetNextRegionTspec(TSpec& tspec, TLink* next_link)
{
    TSpec &tspec_link = TWDATA(this->GetLocalEnd())->tspec;

    // Check adaptation defined by IACD(s)
    list<IACD*>::iterator it_iacd;
    for (it_iacd = adjCapDescriptors.begin(); it_iacd != adjCapDescriptors.end(); it_iacd++)
    {
        // crossing from lower layer to upper layer
        if ((*it_iacd)->lowerLayerSwitchingType == tspec_link.SWtype && (*it_iacd)->lowerLayerEncodingType == tspec_link.ENCtype)
        {
            bool doAdjust = false;
            // adjust to tspec that must be compatible to nex_link 
            if (next_link == NULL)
            {
                doAdjust = true;
            }
            else
            {        
                list<ISCD*>::iterator it_iscd;
                for (it_iscd = next_link->GetSwCapDescriptors().begin(); it_iscd != next_link->GetSwCapDescriptors().end(); it_iscd++)
                {
                    if ((*it_iscd)->switchingType == (*it_iacd)->upperLayerSwitchingType && (*it_iscd)->encodingType == (*it_iacd)->upperLayerEncodingType)
                    {
                        doAdjust = true;
                        break;
                    }
                }
            }
            if (doAdjust)
            {
                tspec.SWtype = (*it_iacd)->upperLayerSwitchingType;
                tspec.ENCtype = (*it_iacd)->upperLayerEncodingType;
                // TODO: Bandwidth adaptation for lower->upper layer
                switch (tspec.SWtype)
                {
                case LINK_IFSWCAP_PSC1:
                case LINK_IFSWCAP_PSC2:
                case LINK_IFSWCAP_PSC3:
                case LINK_IFSWCAP_PSC4:
                case LINK_IFSWCAP_L2SC:
                    //bandwidth constraint unchanged
                    break;
                case LINK_IFSWCAP_TDM:
                    // TODO: ... (unchanged for now)
                    break;
                case LINK_IFSWCAP_LSC:
                    // TODO: ... (unchanged for now)
                    break;
                case LINK_IFSWCAP_FSC:
                    // TODO: ... (unchanged for now)
                    break;
                }
                return true;
            }
        }
        
        //crossing from upper layer to lower layer
        if ((*it_iacd)->upperLayerSwitchingType == tspec_link.SWtype && (*it_iacd)->upperLayerEncodingType == tspec_link.ENCtype)
        {
            bool doAdjust = false;
            // adjust to tspec that must be compatible to nex_link 
            if (next_link == NULL)
            {
                doAdjust = true;
            }
            else
            {        
                list<ISCD*>::iterator it_iscd;
                for (it_iscd = next_link->GetSwCapDescriptors().begin(); it_iscd != next_link->GetSwCapDescriptors().end(); it_iscd++)
                {
                    if ((*it_iscd)->switchingType == (*it_iacd)->lowerLayerSwitchingType && (*it_iscd)->encodingType == (*it_iacd)->lowerLayerEncodingType)
                    {
                        doAdjust = true;
                        break;
                    }
                }
            }
            if (doAdjust)
            {
                tspec.SWtype = (*it_iacd)->lowerLayerSwitchingType;
                tspec.ENCtype = (*it_iacd)->lowerLayerEncodingType;
                // TODO: Bandwidth adaptation for upper->lower layer
                switch (tspec.SWtype)
                {
                    case LINK_IFSWCAP_PSC1:
                    case LINK_IFSWCAP_PSC2:
                    case LINK_IFSWCAP_PSC3:
                    case LINK_IFSWCAP_PSC4:
                    case LINK_IFSWCAP_L2SC:
                        //bandwidth constraint unchanged
                        break;
                    case LINK_IFSWCAP_TDM:
                        // TODO: ... (unchanged for now)
                        break;
                    case LINK_IFSWCAP_LSC:
                        // TODO: ... (unchanged for now)
                        break;
                    case LINK_IFSWCAP_FSC:
                        // TODO: ... (unchanged for now)
                        break;
                }
                return true;
            }
        }
    }


    // Check implicit adaptation
    if (this->remoteLink)
    {
        ISCD* iscd_adapted = NULL;         
        if (this->adjCapDescriptors.size() == 1 && remoteLink->GetAdjCapDescriptors().size() == 1 && this->GetTheISCD()->switchingType != ((TLink*)remoteLink)->GetTheISCD()->switchingType)
            iscd_adapted = ((TLink*)remoteLink)->GetTheISCD();
        else if (this->adjCapDescriptors.size()*remoteLink->GetAdjCapDescriptors().size() > 1)
        {
            list<ISCD*>::iterator iter_iscd = this->remoteLink->GetSwCapDescriptors().begin();
            for (; iter_iscd != this->remoteLink->GetSwCapDescriptors().end(); iter_iscd++)
                if ((*iter_iscd)->switchingType != tspec.SWtype)
                {
                    iscd_adapted = (*iter_iscd);
                    break;
                }
        }
        
        if (iscd_adapted)
        {
            tspec.SWtype = iscd_adapted->switchingType;
            tspec.ENCtype = iscd_adapted->encodingType;

            // TODO: Bandwidth adaptation
            switch (tspec.SWtype)
            {
            case LINK_IFSWCAP_PSC1:
            case LINK_IFSWCAP_PSC2:
            case LINK_IFSWCAP_PSC3:
            case LINK_IFSWCAP_PSC4:
            case LINK_IFSWCAP_L2SC:
                //bandwidth constraint unchanged
                break;
            case LINK_IFSWCAP_TDM:
                // bandwidth constraint unchanged
                // TODO: ?
                break;
            case LINK_IFSWCAP_LSC:
                // bandwidth constraint unchanged
                // TODO: ?
                break;
            case LINK_IFSWCAP_FSC:
                // bandwidth constraint unchanged
                // TODO: ?
                break;
            }
            return true;
        }
    }

    return false;
}


void TLink::InitNextRegionTagSet(TSpec& tspec, ConstraintTagSet &head_waveset)
{
    list<ISCD*>::iterator it_iscd;
    for (it_iscd = this->GetSwCapDescriptors().begin(); it_iscd != this->GetSwCapDescriptors().end(); it_iscd++)
    {
        if (tspec.SWtype == LINK_IFSWCAP_LSC && (*it_iscd)->switchingType == tspec.SWtype && (*it_iscd)->encodingType == tspec.ENCtype)
        {
            ISCD_LSC* iscd = (ISCD_LSC*)(*it_iscd);
            head_waveset = iscd->availableWavelengths;
            // TODO: handle vendor specific info
            VendorSpecificInfoParser* vendorSpecParser = iscd->VendorSpecificInfo();
            if (vendorSpecParser != NULL)
            {
                // TODO: handle vendor specific info                
                LOG_DEBUG(vendorSpecParser->GetType());
            }
            return;
        }
    }
    head_waveset.AddTag(ANY_TAG);
}


void TGraph::AddDomain(TDomain* domain)
{
    tDomains.push_back(domain);
}


void TGraph::AddNode(TDomain* domain, TNode* node)
{
    tNodes.push_back(node);
    domain->AddNode(node);
    node->SetDomain(domain);
}

void TGraph::RemoveNode(TNode* node)
{
    list<TLink*>::iterator itl;
    list<TLink*>& lclLinks = node->GetLocalLinks();
    itl = lclLinks.begin();
    while (itl != lclLinks.end())
    {
        TLink* link = *itl;
        ++itl;
        RemoveLink(link);
    }
    list<TLink*>& rmtLinks = node->GetRemoteLinks();
    itl = rmtLinks.begin();
    while (itl != rmtLinks.end())
    {
        TLink* link = *itl;
        ++itl;
        RemoveLink(link);
    }
    map<string, Port*, strcmpless>::iterator itp;
    map<string, Port*, strcmpless>& ports = node->GetPorts();
    while (itp != ports.end())
    {
        TPort* port = (TPort*)(*itp).second;
        ++itp;
        RemovePort(port);
    }
    node->GetDomain()->GetNodes().erase(node->GetName());
    tNodes.remove(node);
}

void TGraph::AddPort(TNode* node, TPort* port)
{
    tPorts.push_back(port);
    node->AddPort(port);
    port->SetNode(node);
}

void TGraph::RemovePort(TPort* port)
{
    map<string, Link*, strcmpless>& links = port->GetLinks();
    map<string, Link*, strcmpless>::iterator itl;
    itl = links.begin();
    while (itl != links.end())
    {
        TLink* link = (TLink*)(*itl).second;
        ++itl;
        RemoveLink(link);
    }
    port->GetNode()->GetPorts().erase(port->GetName());
    tPorts.remove(port);
}


void TGraph::AddLink(TPort* port, TLink* link)
{
    tLinks.push_back(link);
    port->AddLink(link);
    link->SetPort(port);
}


void TGraph::AddLink(TNode* node, TLink* link)
{
    if (link->GetPort() == NULL)
        tLinks.push_back(link);
    node->AddLocalLink(link);
    link->SetLocalEnd(node);
}

void TGraph::RemoveLink(TLink* link)
{
    link->GetPort()->GetLinks().erase(link->GetName());
    if (link->GetLocalEnd())
    {
        list<TLink*>::iterator itLink = link->GetLocalEnd()->GetLocalLinks().begin();
        for (; itLink != link->GetLocalEnd()->GetLocalLinks().end(); itLink++)
        {
            if ((*itLink) == link)
                itLink = link->GetLocalEnd()->GetLocalLinks().erase(itLink);
        }
    }
    if (link->GetRemoteEnd())
    {
        list<TLink*>::iterator itLink = link->GetRemoteEnd()->GetRemoteLinks().begin();
        for (; itLink != link->GetRemoteEnd()->GetRemoteLinks().end(); itLink++)
        {
            if ((*itLink) == link)
                itLink = link->GetRemoteEnd()->GetRemoteLinks().erase(itLink);
        }
    }
    if (link->GetRemoteLink() != NULL)
        link->GetRemoteLink()->SetRemoteLink(NULL);
    tLinks.remove(link);
}



TDomain* TGraph::LookupDomainByName(string& name)
{
    list<TDomain*>::iterator itd = tDomains.begin();
    for (; itd != tDomains.end(); itd++)
    {
        TDomain* td = *itd;
        if (strcasecmp(td->GetName().c_str(), name.c_str()) == 0)
            return td;
    }
    return NULL;
}



TDomain* TGraph::LookupDomainByURN(string& urn)
{
    string domainName = GetUrnField(urn, "domain");
    if (domainName.empty())
        return NULL;
    return LookupDomainByName(domainName);
}


TNode* TGraph::LookupNodeByURN(string& urn)
{
    TDomain* td = LookupDomainByURN(urn);
    if (td == NULL)
        return NULL;
    string nodeName = GetUrnField(urn, "node");
    map<string, Node*, strcmpless>::iterator itn = td->GetNodes().find(nodeName);
    if (itn == td->GetNodes().end())
        return NULL;
    return (TNode*)(*itn).second;
}


TPort* TGraph::LookupPortByURN(string& urn)
{
    TNode* tn = LookupNodeByURN(urn);
    if (tn == NULL)
        return NULL;
    string portName = GetUrnField(urn, "port");
    map<string, Port*, strcmpless>::iterator itp = tn->GetPorts().find(portName);
    if (itp == tn->GetPorts().end())
        return NULL;
    return (TPort*)(*itp).second;
}


TLink* TGraph::LookupLinkByURN(string& urn)
{
    TPort* tp = LookupPortByURN(urn);
    if (tp == NULL)
        return NULL;
    string linkName = GetUrnField(urn, "link");
    map<string, Link*, strcmpless>::iterator itl = tp->GetLinks().find(linkName);
    if (itl == tp->GetLinks().end())
        return NULL;
    return (TLink*)(*itl).second;
}


void TGraph::LoadPath(list<TLink*> path)
{
    string domainName, nodeName, portName, linkName;
    char buf[256];
    list<TLink*>::iterator itL;
    TLink* lastLink = NULL;
    for (itL = path.begin(); itL != path.end(); itL++)
    {
        TLink* link = *itL;
        string urn = link->GetName();
        if (urn.find("urn") == string::npos)
        {
            linkName = link->GetName();
            portName = link->GetPort()->GetName();
            nodeName = link->GetPort()->GetNode()->GetName();
            domainName = link->GetPort()->GetNode()->GetDomain()->GetName();
            urn = "urn:ogf:network:domain=";
            urn += domainName;
            urn += ":node=";
            urn += nodeName;
            urn += ":port=";
            urn += portName;
            urn += ":link=";
            urn += linkName;
        }
        else
        {
            ParseFQUrn(urn, domainName, nodeName, portName, linkName);
            if (domainName.length() == 0 || nodeName.length() == 0 || portName.length() == 0 || linkName.length() == 0)
            {
                snprintf(buf, 256, "TGraph::LoadPath raises Excaption: invalid link urn '%s'", urn.c_str());
                throw TEDBException(buf);
            }
            if (LookupLinkByURN(urn) != NULL || LookupPortByURN(urn) != NULL)
            {
                snprintf(buf, 256, "TGraph::LoadPath raises Excaption: duplicate link '%s' in path", urn.c_str());
                throw TEDBException(buf);
            }
            link->SetName(linkName);
        }
        TDomain* domain = LookupDomainByName(domainName);
        if (domain == NULL)
        {
            domain = new TDomain(0, domainName);
            AddDomain(domain);
        }
        TNode* node = LookupNodeByURN(urn);
        if (node == NULL)
        {
            node = new TNode(0, nodeName);
            AddNode(domain, node);
            if (lastLink != NULL)
            {
                lastLink->SetRemoteEnd(node);
                lastLink->SetRemoteLink(link);
                link->SetRemoteEnd((TNode*)lastLink->GetPort()->GetNode());
                link->SetRemoteLink(lastLink);
            }
        }
        TPort* port = new TPort(link->GetId(), portName);
        AddPort(node, port);
        AddLink(port, link);
        lastLink = link;
    }
}


TGraph* TGraph::Clone()
{
    TGraph* tg = new TGraph(name);
    list<TDomain*>::iterator itd = this->tDomains.begin();
    for (; itd != this->tDomains.end(); itd++)
    {
        TDomain* td = (*itd)->Clone(true);
        tg->GetDomains().push_back(td);
        map<string, Node*, strcmpless>::iterator itn = td->GetNodes().begin();
        for (; itn != td->GetNodes().end(); itn++)
        {
            TNode* tn = (TNode*)(*itn).second;
            tg->GetNodes().push_back(tn);
            map<string, Port*, strcmpless>::iterator itp = tn->GetPorts().begin();
            for (; itp != tn->GetPorts().end(); itp++)
            {
                TPort* tp = (TPort*)(*itp).second;
                tg->GetPorts().push_back(tp);
                map<string, Link*, strcmpless>::iterator itl = tp->GetLinks().begin();
                for (; itl != tp->GetLinks().end(); itl++)
                    tg->GetLinks().push_back((TLink*)(*itl).second);
            }
            list<TLink*>::iterator itll = tn->GetLocalLinks().begin();
            for (; itll != tn->GetLocalLinks().end(); itll++)
            {
                TLink* tl = (*itll);
                if (tl->GetPort() == NULL)
                    tg->GetLinks().push_back(tl);
            }
        }
    }
    return tg;
}



void TGraph::LogDump()
{
    char buf[1024000]; //up to 1000K
    char str[256];
    strcpy(buf, "TEWG Dump...\n");
    list<TDomain*>::iterator itd = this->tDomains.begin();
    for (; itd != this->tDomains.end(); itd++)
    {
        TDomain* td = (*itd);
        snprintf(str, 256, "<domain id=%s>\n", td->GetName().c_str());
        strcat(buf, str);
        map<string, Node*, strcmpless>::iterator itn = td->GetNodes().begin();
        for (; itn != td->GetNodes().end(); itn++)
        {
            TNode* tn = (TNode*)(*itn).second;
            snprintf(str, 256, "\t<node id=%s>\n", tn->GetName().c_str());
            strcat(buf, str);
            map<string, Port*, strcmpless>::iterator itp = tn->GetPorts().begin();
            for (; itp != tn->GetPorts().end(); itp++)
            {
                TPort* tp = (TPort*)(*itp).second;
                snprintf(str, 256, "\t\t<port id=%s>\n", tp->GetName().c_str());
                strcat(buf, str);
                map<string, Link*, strcmpless>::iterator itl = tp->GetLinks().begin();
                for (; itl != tp->GetLinks().end(); itl++) 
                {
                    TLink* tl = (TLink*)(*itl).second;
                    snprintf(str, 256, "\t\t\t<link id=%s>\n", tl->GetName().c_str());
                    strcat(buf, str);
                    if (tl->GetRemoteLink())
                    {
                        snprintf(str, 256, "\t\t\t\t<remoteLinkId>domain=%s:node=%s:port=%s:link=%s</remoteLinkId>\n",  
                            tl->GetRemoteLink()->GetPort()->GetNode()->GetDomain()->GetName().c_str(),
                            tl->GetRemoteLink()->GetPort()->GetNode()->GetName().c_str(),
                            tl->GetRemoteLink()->GetPort()->GetName().c_str(), 
                            tl->GetRemoteLink()->GetName().c_str());
                        strcat(buf, str);
                    }
                    snprintf(str, 256, "\t\t\t\t<AvailableBandwidth>%llu</AvailableBandwidth>\n", tl->GetAvailableBandwidth());
                    strcat(buf, str);
                    if (tl->GetTheISCD())
                    {
                        snprintf(str, 256, "\t\t\t\t<SwitchingCapabilityDescriptors> <switchingcapType=%d><encodingType=%d><capacity=%llu> </SwitchingCapabilityDescriptors>\n",  
                            tl->GetTheISCD()->switchingType,
                            tl->GetTheISCD()->encodingType,
                            tl->GetTheISCD()->capacity);
                        strcat(buf, str);
                    }
                    if (tl->GetDeltaList().size() > 0)
                    {
                        strcat(buf, "\t\t\t\t<DeltaList>");
                        list<TDelta*>::iterator itD = tl->GetDeltaList().begin();
                        for (; itD != tl->GetDeltaList().end(); itD++)
                        {
                            snprintf(str, 256, "[bw=%llu:%d-%d] ", ((TLinkDelta*)(*itD))->GetBandwidth(), (int)(*itD)->GetStartTime(), (int)(*itD)->GetEndTime());
                            strcat(buf, str);
                        }
                        strcat(buf, "</DeltaList>\n");
                    }
                    snprintf(str, 256, "\t\t\t</link>\n");
                    strcat(buf, str);
                }
                snprintf(str, 256, "\t\t</port>\n");
                strcat(buf, str);
            }
            snprintf(str, 256, "\t</node>\n");
            strcat(buf, str);
        }
        snprintf(str, 256, "</domain>\n");
        strcat(buf, str);
    }    
    LOG_DEBUG(buf);
}

bool cmp_tpath(TPath* p1, TPath* p2)
{
    return (p1->GetCost() < p2->GetCost());
}


TPath::~TPath()
{
    if (independent)
    {
        list<TLink*>::iterator itL = path.begin();
        for (; itL != path.end(); itL++)
            delete *itL;
        path.clear();
    }
    list<TSchedule*>::iterator itS = schedules.begin();
    for (; itS != schedules.end(); itS++)
        delete *itS;
    schedules.clear();
    if (bag != NULL)
        delete bag;
}

void TPath::ExpandWithRemoteLinks()
{
    TLink *L, *remoteL;
    if (path.size() == 0)
        return;
    list<TLink*>::iterator iterL = path.begin();
    while(iterL != path.end())
    {
        L = *iterL;
        list<TLink*>::iterator iterNextL = iterL; 
        ++iterNextL;
        remoteL = (TLink*)L->GetRemoteLink();
        if (iterNextL == path.end())
            break;
        if ((*iterNextL) != remoteL && L->GetLocalEnd() != (*iterNextL)->GetLocalEnd())
        {
            path.insert(iterNextL, (TLink*)L->GetRemoteLink());
        }
        iterL = iterNextL;
    }
}

// verify constrains of vlantag, wavelength and cross-layer adaptation (via TSpec) 
// TODO: support for MLN request types (currently only Ethernet-over-WDM altough other cross-layers are computed but not accounted in results)
bool TPath::VerifyTEConstraints(TServiceSpec& ingTSS,TServiceSpec& egrTSS)//u_int32_t& srcVtag, u_int32_t& dstVtag, u_int32_t& wave, TSpec& tspec) 
{
    TLink* L;
    list<TLink*>::iterator iterL;
    ConstraintTagSet head_vtagset(MAX_VLAN_NUM), next_vtagset(MAX_VLAN_NUM);
    ConstraintTagSet head_vtagset_trans(MAX_VLAN_NUM), next_vtagset_trans(MAX_VLAN_NUM);
    ConstraintTagSet init_vtagset(MAX_VLAN_NUM);
    bool no_vtag = false, no_vtag_trans = false;
    ConstraintTagSet head_waveset(MAX_WAVE_NUM), next_waveset(MAX_WAVE_NUM);

    if (path.size() == 0)
        return false;

    // initializing tspecs
    for (iterL = path.begin(); iterL != path.end(); iterL++)
    {
        L = *iterL;
        if (L->GetLocalEnd())
            TWDATA(L->GetLocalEnd())->tspec.Update(ingTSS.SWtype, ingTSS.ENCtype, ingTSS.Bandwidth);
        if (L->GetRemoteEnd())
            TWDATA(L->GetRemoteEnd())->tspec.Update(ingTSS.SWtype, ingTSS.ENCtype, ingTSS.Bandwidth);
        TWDATA(L)->tspec.Update(ingTSS.SWtype, ingTSS.ENCtype, ingTSS.Bandwidth);
    }

    // initializing vtags and wavelengths
    u_int32_t srcVtag = ANY_TAG;
    if (ingTSS.SWtype == LINK_IFSWCAP_L2SC && ingTSS.ENCtype == LINK_IFSWCAP_ENC_ETH)
    {
        head_vtagset = ingTSS.GetTagSet();
        if (!head_vtagset.IsEmpty() && !head_vtagset.HasAnyTag())
            srcVtag = ingTSS.GetTagSet().LowestTag();
    }
    else         
        head_vtagset.Clear();
    head_vtagset_trans = head_vtagset;
    init_vtagset.Clear();
    head_waveset.Clear();

    // verifying path constraints
    for (iterL = path.begin(); iterL != path.end(); iterL++)
    {
        L = (*iterL);
        if (L->GetLocalEnd() == NULL || L->GetRemoteEnd()==NULL || L->GetSwCapDescriptors().size() == 0)
            return false;

        if (!L->IsAvailableForTspec(TWDATA(L->GetLocalEnd())->tspec))
            return false;
        // check continous (common) vlan tags, skip if no_vtag has already been set true
        if (!no_vtag && !head_vtagset.IsEmpty())
        {
            L->ProceedByUpdatingVtags(head_vtagset, next_vtagset, false);
            if (next_vtagset.IsEmpty())
                no_vtag = true;
            else
                head_vtagset = next_vtagset;
            if (init_vtagset.IsEmpty())
                init_vtagset = next_vtagset;
        }
        // some vlan tag must be avaible for the first link. Even vlan translation cannot help here
        if (no_vtag && iterL == path.begin())
            return false;
        // check vlan tags with translation, , skip if no_vtag_trans has already been set true
        if (!no_vtag_trans && !head_vtagset_trans.IsEmpty())
        {
            L->ProceedByUpdatingVtags(head_vtagset_trans, next_vtagset_trans, true);
            if (next_vtagset_trans.IsEmpty())
                no_vtag_trans = true;
            else
                head_vtagset_trans = next_vtagset_trans;
        }
        if (no_vtag && no_vtag_trans)
            return false;
        // we currently do not consider wavelength translation
        if (TWDATA(L->GetLocalEnd())->tspec.SWtype == LINK_IFSWCAP_LSC && !head_waveset.IsEmpty())
        {
            L->ProceedByUpdatingWaves(head_waveset, next_waveset, true);
            if (next_waveset.IsEmpty())
                return false;
            head_waveset = next_waveset;
        }
        // prepare Tspec for headnode of next link
        list<TLink*>::iterator iterNext = iterL;
        iterNext++;
        if (iterNext == path.end())
            continue;
        TLink* nextL = *iterNext;
        if (L->CrossingRegionBoundary(TWDATA(L->GetLocalEnd())->tspec, nextL))
        {
            L->GetNextRegionTspec(TWDATA(nextL->GetLocalEnd())->tspec, nextL);
            // handling LSC region entry case only for now
            if (TWDATA(nextL->GetLocalEnd())->tspec.SWtype == LINK_IFSWCAP_LSC)
                L->InitNextRegionTagSet(TWDATA(nextL->GetLocalEnd())->tspec, head_waveset);
        }       
        else
        {
            TWDATA(nextL->GetLocalEnd())->tspec = TWDATA(L->GetLocalEnd())->tspec;
        }
    }

    u_int32_t dstVtag = ANY_TAG;
    if (egrTSS.SWtype == LINK_IFSWCAP_L2SC && egrTSS.ENCtype == LINK_IFSWCAP_ENC_ETH
        && !egrTSS.GetTagSet().IsEmpty() && !egrTSS.GetTagSet().HasAnyTag())
        dstVtag = egrTSS.GetTagSet().LowestTag();
    // pick src and dst vtags from two sets of head_set and next_set (randomized by default)
    // try using common (continuous) vlan tag if possible, otherwise (no_vtag==true) use translated diff vlan
    if (no_vtag || (dstVtag != ANY_TAG && !next_vtagset.HasTag(dstVtag)))
    {
        assert(!no_vtag_trans && next_vtagset_trans.Size() > 0);
        if (srcVtag == ANY_TAG)
        {
            L = path.front();
            if (init_vtagset.IsEmpty())
                return false;
            else if (init_vtagset.HasAnyTag())
                srcVtag = next_vtagset_trans.RandomTag();
            else
                srcVtag = init_vtagset.RandomTag();
        }
        if (dstVtag == ANY_TAG)
            dstVtag = next_vtagset_trans.RandomTag(); 
        else if (!next_vtagset_trans.HasTag(dstVtag))
            return false;
    }
    else
    {
        assert(next_vtagset.Size() > 0);    
        if (dstVtag != ANY_TAG && !next_vtagset.HasTag(dstVtag)) 
            return false;
        else if (srcVtag == ANY_TAG && dstVtag != ANY_TAG)
            srcVtag = dstVtag;
        else if (srcVtag != ANY_TAG && dstVtag == ANY_TAG)
            dstVtag = srcVtag;
        else if (srcVtag == ANY_TAG && dstVtag == ANY_TAG)
            dstVtag = srcVtag = next_vtagset.RandomTag(); 
    }
    //finalize vlans
    ingTSS.GetVlanSet().Clear();
    if (ingTSS.SWtype == LINK_IFSWCAP_L2SC && ingTSS.ENCtype == LINK_IFSWCAP_ENC_ETH)
        ingTSS.GetVlanSet().AddTag(srcVtag);
    egrTSS.GetVlanSet().Clear();
    if (egrTSS.SWtype == LINK_IFSWCAP_L2SC && egrTSS.ENCtype == LINK_IFSWCAP_ENC_ETH)
        egrTSS.GetVlanSet().AddTag(dstVtag);
    // pick wavelength (randomized)
    if (!next_waveset.IsEmpty())
    {
        u_int32_t wavelength = next_waveset.RandomTag();
        ingTSS.GetWavelengthSet().AddTag(wavelength);
        egrTSS.GetWavelengthSet().AddTag(wavelength);
    }
    return true;
}

// Note: does not guarantee right order of inclusion list
bool TPath::VerifyHopInclusionList(list<string>& inclusionList)
{
     if (path.size() == 0)
        return false;
     if (inclusionList.size() == 0)
        return true;
    TLink* L;
    list<TLink*>::iterator iterL;
    list<string>::iterator iterI;
    for (iterI = inclusionList.begin(); iterI != inclusionList.end(); iterI++) 
    {
        string& inclusionUrn = *iterI;
        for (iterL = path.begin(); iterL != path.end(); iterL++) 
        {
            L = *iterL;
            if (L->VerifyContainUrn(inclusionUrn))
                break; // inclusionUrn found on forward link
            if (L->GetRemoteLink() != NULL && ((TLink*)L->GetRemoteLink())->VerifyContainUrn(inclusionUrn))
                break; // inclusionUrn found on backward link
        }
        if (iterL == path.end())
            return false; // this inclusionUrn not found in the path
    }
    return true;
}

void TPath::UpdateLayerSpecInfo(TServiceSpec& ingTSS, TServiceSpec& egrTSS)
{
    TLink* L;
    list<TLink*>::iterator iterL;
    u_int32_t srcVtag = ingTSS.GetVlanSet().LowestTag();
    u_int32_t dstVtag = egrTSS.GetVlanSet().LowestTag();
    u_int32_t srcWave = (ingTSS.GetWavelengthSet().IsEmpty()? 0 : ingTSS.GetWavelengthSet().LowestTag());
    u_int32_t dstWave = (egrTSS.GetWavelengthSet().IsEmpty()? 0 : egrTSS.GetWavelengthSet().LowestTag());
    
    //// update cross-layer info -- strip unused ISCD(s) and IACD(s) from final path hops
    for (iterL = path.begin(); iterL != path.end(); iterL++)
    {
        L = *iterL;
        list<TLink*>::iterator iterN = iterL; ++iterN;
        list<ISCD*>::iterator it;
        ISCD* iscdL = NULL; ISCD* iscdN = NULL;
        list<IACD*>::iterator ita;
        IACD* iacd = NULL;
        TSpec& tspecL = TWDATA(L->GetLocalEnd())->tspec;
        // first hop
        if (iterL == path.begin())
        {
            L->GetAdjCapDescriptors().clear();
        }            
        // last hop
        if (iterN == path.end())
        {
            for (it = L->GetSwCapDescriptors().begin(); it !=  L->GetSwCapDescriptors().end(); it++)
            {
                if ((*it)->switchingType != tspecL.SWtype || (*it)->encodingType != tspecL.ENCtype)
                    it = L->GetSwCapDescriptors().erase(it);
            }
            L->GetAdjCapDescriptors().clear();
            break;
        }
        TLink* nextL = *iterN;
        TSpec& tspecN = TWDATA(nextL->GetLocalEnd())->tspec;
        // same layer/region handling: only one ISCD is left
        if (tspecL.SWtype == tspecN.SWtype && tspecL.ENCtype == tspecN.ENCtype) 
        {
            for (it = L->GetSwCapDescriptors().begin(); it !=  L->GetSwCapDescriptors().end(); it++)
            {
                if ((*it)->switchingType != tspecL.SWtype || (*it)->encodingType != tspecL.ENCtype)
                    it = L->GetSwCapDescriptors().erase(it);
            }
            continue;
        }
        // example current link in a cross-layer case
        for (it = L->GetSwCapDescriptors().begin(); it !=  L->GetSwCapDescriptors().end(); it++)
        {
            if ((*it)->switchingType == tspecL.SWtype && (*it)->encodingType == tspecL.ENCtype)
                iscdL = *it;
            if ((*it)->switchingType == tspecN.SWtype && (*it)->encodingType == tspecN.ENCtype)
                iscdN = *it;
        }
        for (ita = L->GetAdjCapDescriptors().begin(); ita !=  L->GetAdjCapDescriptors().end(); ita++)
        {
            if (((*ita)->lowerLayerSwitchingType == tspecL.SWtype && (*ita)->lowerLayerEncodingType == tspecL.ENCtype
                && (*ita)->upperLayerSwitchingType == tspecN.SWtype && (*ita)->upperLayerEncodingType == tspecN.ENCtype)
                ||((*ita)->lowerLayerSwitchingType == tspecN.SWtype && (*ita)->lowerLayerEncodingType == tspecN.ENCtype
                && (*ita)->upperLayerSwitchingType == tspecL.SWtype && (*ita)->upperLayerEncodingType == tspecL.ENCtype)) 
            {
                iacd = *ita;
                break;
            }
        }
        if (iscdL == NULL)
            throw TCEException((char*)"TPath::UpdateLayerSpecInfo(): Hop missing valid ISCD");
        L->GetSwCapDescriptors().clear(); // mem leak
        L->GetAdjCapDescriptors().clear(); // mem leak
        // no adjust or implicit adjust case
        if (iscdN == NULL || iacd == NULL)
        {
            L->GetSwCapDescriptors().push_back(iscdL);
        }
        else // explicit adjust case
        {
            L->GetSwCapDescriptors().push_back(iscdL);
            L->GetSwCapDescriptors().push_back(iscdN);
            L->GetAdjCapDescriptors().push_back(iacd);
        }

        //  example next link in a cross-layer case
        iscdL = NULL; iscdN = NULL; iacd = NULL;
        for (it = nextL->GetSwCapDescriptors().begin(); it != nextL->GetSwCapDescriptors().end(); it++)
        {
            if ((*it)->switchingType == tspecL.SWtype && (*it)->encodingType == tspecL.ENCtype)
                iscdL = *it;
            if ((*it)->switchingType == tspecN.SWtype && (*it)->encodingType == tspecN.ENCtype)
                iscdN = *it;
        }
        for (ita = nextL->GetAdjCapDescriptors().begin(); ita != nextL->GetAdjCapDescriptors().end(); ita++)
        {
            if (((*ita)->lowerLayerSwitchingType == tspecL.SWtype && (*ita)->lowerLayerEncodingType == tspecL.ENCtype
                && (*ita)->upperLayerSwitchingType == tspecN.SWtype && (*ita)->upperLayerEncodingType == tspecN.ENCtype)
                ||((*ita)->lowerLayerSwitchingType == tspecN.SWtype && (*ita)->lowerLayerEncodingType == tspecN.ENCtype
                && (*ita)->upperLayerSwitchingType == tspecL.SWtype && (*ita)->upperLayerEncodingType == tspecL.ENCtype)) 
            {
                iacd = *ita;
                break;
            }
        }
        if (iscdN == NULL)
            throw TCEException((char*)"TPath::UpdateLayerSpecInfo(): Hop missing valid ISCD");
        nextL->GetSwCapDescriptors().clear(); // mem leak
        nextL->GetAdjCapDescriptors().clear(); // mem leak
        // no adjust or implicit adjust case
        if (iscdL == NULL || iacd == NULL)
        {
            nextL->GetSwCapDescriptors().push_back(iscdN);
        }
        else // explicit adjust case
        {
            nextL->GetSwCapDescriptors().push_back(iscdL);
            nextL->GetSwCapDescriptors().push_back(iscdN);
            nextL->GetAdjCapDescriptors().push_back(iacd);
        }
        ++iterL;
    }    


    //// update Layer2 VLANs    
    // TODO: vlanRange instead of single tags ?  -- (treat  availableVtagRange different ?)
    bool forwardContinued = true;
    // assign srcVtag to suggestedVlanTags along the path in foward direction
    for (iterL = path.begin(); iterL != path.end(); iterL++)
    {
        L = *iterL;
        list<ISCD*>::iterator it;
        ISCD_L2SC* iscd = NULL;
        for (it = L->GetSwCapDescriptors().begin(); it !=  L->GetSwCapDescriptors().end(); it++)
        {
            if ((*it)->switchingType != LINK_IFSWCAP_L2SC || (*it)->encodingType != LINK_IFSWCAP_ENC_ETH)
                continue;
            iscd = (ISCD_L2SC*)(*it);
            break;
        }
        if (!iscd)
            continue;
        if (forwardContinued && iscd->availableVlanTags.HasTag(srcVtag))
        {
            iscd->availableVlanTags.Clear();
            iscd->suggestedVlanTags.Clear();
            iscd->availableVlanTags.AddTag(srcVtag);
            iscd->suggestedVlanTags.AddTag(srcVtag);
        }
        else 
        {
            iscd->availableVlanTags.Clear();
            iscd->suggestedVlanTags.Clear();
            forwardContinued = false;
        }
    }
    if (dstVtag != srcVtag)
    {
        // assign dstVtag to suggestedVlanTags along the path in reverse direction
        list<TLink*>::reverse_iterator iterR;
        for (iterR = path.rbegin(); iterR != path.rend(); iterR++)
        {
            L = *iterR;
            list<ISCD*>::iterator it;
            ISCD_L2SC* iscd = NULL;
            for (it = L->GetSwCapDescriptors().begin(); it !=  L->GetSwCapDescriptors().end(); it++)
            {
                if ((*it)->switchingType != LINK_IFSWCAP_L2SC || (*it)->encodingType != LINK_IFSWCAP_ENC_ETH)
                    continue;
                iscd = (ISCD_L2SC*)(*it);
                break;
            }
            if (!iscd)
                continue;
            if (iterR == path.rbegin())
            {
                iscd->availableVlanTags.Clear();
                iscd->suggestedVlanTags.Clear();
            }
            else if (!iscd->suggestedVlanTags.IsEmpty()|| iscd->availableVlanTags.HasTag(dstVtag))
                break;
            iscd->availableVlanTags.AddTag(dstVtag);
            iscd->suggestedVlanTags.AddTag(dstVtag);
        }
    }

    //// update LSC wavelengths
    if (srcWave != 0 && dstWave != 0) {
        forwardContinued = true;
        for (iterL = path.begin(); iterL != path.end(); iterL++)
        {
            L = *iterL;
            list<ISCD*>::iterator it;
            ISCD_LSC* iscd = NULL;
            for (it = L->GetSwCapDescriptors().begin(); it !=  L->GetSwCapDescriptors().end(); it++)
            {
                if ((*it)->switchingType != LINK_IFSWCAP_LSC)
                    continue;
                iscd = (ISCD_LSC*)(*it);
                break;
            }
            if (!iscd)
                continue;
            if (forwardContinued && iscd->availableWavelengths.HasTag(srcWave))
            {
                iscd->availableWavelengths.Clear();
                iscd->suggestedWavelengths.Clear();
                iscd->availableWavelengths.AddTag(srcWave);
                iscd->suggestedWavelengths.AddTag(srcWave);
            }
            else 
            {
                iscd->availableWavelengths.Clear();
                iscd->suggestedWavelengths.Clear();
                forwardContinued = false;
            }
            VendorSpecificInfoParser* vendorSpecInfo = iscd->VendorSpecificInfo();
            if (vendorSpecInfo == NULL)
                continue;
            if (vendorSpecInfo->GetType() == "InfineraDTNSpecificInfo:tributaryInfo")
            {
                // TODO: handle vendor specific info for InfineraDTNSpecificInfo:tributaryInfo
                // pick the resulting OTN object
                // TODO: Design
                // 1.  Format result into XML and load into ISCD::vendorSpecificInfoXml (xmlNodePtr type)
                // 2.  API only need to extract the XML from (xmlNodePtr) and parse as plain text to tcePCE 
                // Add tools to facilitate both 1 and 2
            }
            else if (vendorSpecInfo->GetType() == "InfineraDTNSpecificInfo:wavebandMuxInfo")
            {
                // TODO: handle vendor specific info for InfineraDTNSpecificInfo:wavebandMuxInfo
                // pick the OTN object
            }
        }
        if (dstWave != srcWave)
        {
            // assign dstWave to suggestedWavelength along the path in reverse direction
            list<TLink*>::reverse_iterator iterR;
            for (iterR = path.rbegin(); iterR != path.rend(); iterR++)
            {
                L = *iterR;
                list<ISCD*>::iterator it;
                ISCD_LSC* iscd = NULL;
                for (it = L->GetSwCapDescriptors().begin(); it !=  L->GetSwCapDescriptors().end(); it++)
                {
                    if ((*it)->switchingType != LINK_IFSWCAP_LSC)
                        continue;
                    iscd = (ISCD_LSC*)(*it);
                    break;
                }
                if (!iscd)
                    continue;
                if (iterR == path.rbegin())
                {
                    iscd->availableWavelengths.Clear();
                    iscd->suggestedWavelengths.Clear();
                }
                else if (!iscd->suggestedWavelengths.IsEmpty()|| iscd->availableWavelengths.HasTag(dstVtag))
                    break;
                iscd->availableWavelengths.AddTag(dstVtag);
                iscd->suggestedWavelengths.AddTag(dstVtag);
            }
        }
    }
}

BandwidthAvailabilityGraph* TPath::CreatePathBAG(time_t start, time_t end)
{
    assert(path.size() > 0);
    list<TLink*>::iterator itL = path.begin();
    AggregateDeltaSeries* ads = (AggregateDeltaSeries*)((*itL)->GetWorkData()->GetData("ADS"));
    if (ads == NULL)
    {
        ads = new AggregateDeltaSeries;
        list<TDelta*>::iterator itD;
        for (itD = (*itL)->GetDeltaList().begin(); itD != (*itL)->GetDeltaList().end(); itD++)
        {
            TDelta* delta = *itD;
            ads->AddDelta(delta);
        }
        (*itL)->GetWorkData()->SetData("ADS", ads);
    }
    ads = ads->Duplicate();
    itL++;
    u_int64_t capacity = (*itL)->GetMaxReservableBandwidth();
    for (; itL != path.end(); itL++)
    {
        if ((*itL)->GetWorkData()->GetData("ADS") == NULL)
        {
            AggregateDeltaSeries* ads2 = new AggregateDeltaSeries;
            list<TDelta*>::iterator itD;
            for (itD = (*itL)->GetDeltaList().begin(); itD != (*itL)->GetDeltaList().end(); itD++)
            {
                TDelta* delta = *itD;
                ads2->AddDelta(delta);
            }
            (*itL)->GetWorkData()->SetData("ADS", ads2);
        }
        ads->Join(*(AggregateDeltaSeries*)((*itL)->GetWorkData()->GetData("ADS")));
        if (capacity > (*itL)->GetMaxReservableBandwidth())
            capacity = (*itL)->GetMaxReservableBandwidth();
    }
    BandwidthAvailabilityGraph* bag = new BandwidthAvailabilityGraph();
    bag->LoadADS(*ads, start, end, capacity);
    return bag;
}

void TPath::CreateLinkBAG(time_t start, time_t end)
{
    assert(path.size() > 0);
    list<TLink*>::iterator itL = path.begin();
    for (; itL != path.end(); itL++)
    {
        AggregateDeltaSeries* ads = (AggregateDeltaSeries*)((*itL)->GetWorkData()->GetData("ADS"));
        if (ads == NULL) {
            ads = new AggregateDeltaSeries;
            list<TDelta*>::iterator itD;
            for (itD = (*itL)->GetDeltaList().begin(); itD != (*itL)->GetDeltaList().end(); itD++)
            {
                TDelta* delta = *itD;
                ads->AddDelta(delta);
            }
        }
        u_int64_t capacity = (*itL)->GetMaxReservableBandwidth();
        BandwidthAvailabilityGraph* bag = new BandwidthAvailabilityGraph();
        bag->LoadADS(*ads, start, end, capacity);
        (*itL)->SetBAG(bag);
    }
}

TPath* TPath::Clone(bool doExpandRemoteLink)
{
    TPath* P = new TPath;
    P->SetCost(this->cost);
    P->SetIndependent(this->independent);
    list<TLink*>::iterator itL;
    TLink* lastLink = NULL;
    for (itL = this->path.begin(); itL != this->path.end(); itL++)
    {
        TLink* L = (*itL)->Clone();
        L->SetLocalEnd((*itL)->GetLocalEnd());
        L->SetRemoteEnd((*itL)->GetRemoteEnd());
        if (doExpandRemoteLink && lastLink 
            && L->GetPort()->GetNode() != lastLink->GetPort()->GetNode() 
            && lastLink->GetRemoteLink() && lastLink->GetRemoteLink() != L)
        {
            TLink* rL = ((TLink*)lastLink->GetRemoteLink())->Clone();
            rL->SetLocalEnd(((TLink*)lastLink->GetRemoteLink())->GetLocalEnd());
            rL->SetRemoteEnd(((TLink*)lastLink->GetRemoteLink())->GetRemoteEnd());
            lastLink->SetRemoteLink(rL);
            rL->SetRemoteLink(lastLink);
            P->GetPath().push_back(rL);
        }
        lastLink = L;
        P->GetPath().push_back(L);
    }
    if (this->schedules.size() > 0) 
    {
        list<TSchedule*>::iterator itS = this->schedules.begin();
        for ( ; itS != this->schedules.end(); itS++)
        {
            P->GetSchedules().push_back((*itS)->Clone());    
        }
    }
    if (bag != NULL)
        P->bag = bag->Clone();
    // TODO: set all port, lclEnd and rmtEnd to NULL
    return P;
}

void TPath::LogDump()
{
    char buf[10240]; //up to 10K
    char str[256];
    sprintf(buf, "TPath (cost=%lf):", this->cost);
    if (path.size() == 0)
    {
        strcat(buf," empty (No links)");
    }
    else
    {  
        list<TLink*>::iterator itL = path.begin();
        for (; itL != path.end(); itL++)
        {
            TLink* L = *itL;
            snprintf(str, 256, " ->%s:%s:%s:%s",
                L->GetPort()->GetNode()->GetDomain()->GetName().c_str(),
                L->GetPort()->GetNode()->GetName().c_str(),
                L->GetPort()->GetName().c_str(), 
                L->GetName().c_str());
            strcat(buf, str);
            list<ISCD*>::iterator it;
            ISCD_L2SC* iscd_l2sc = NULL;
            ISCD_LSC* iscd_lsc = NULL;
            for (it = L->GetSwCapDescriptors().begin(); it !=  L->GetSwCapDescriptors().end(); it++)
            {
                if ((*it)->switchingType == LINK_IFSWCAP_L2SC && (*it)->encodingType == LINK_IFSWCAP_ENC_ETH)
                    iscd_l2sc = (ISCD_L2SC*)(*it);
                else if ((*it)->switchingType == LINK_IFSWCAP_LSC)
                    iscd_lsc = (ISCD_LSC*)(*it);
            }
            if (iscd_l2sc)
            {
                snprintf(str, 256, " (suggestedVlan:%s) ", iscd_l2sc->suggestedVlanTags.GetRangeString().c_str());
                strcat(buf, str);
            }    
            if (iscd_lsc && iscd_lsc->channelRepresentation == ITU_GRID_50GHZ) // for test 
            {
                snprintf(str, 256, " (suggestedWave:%s) ", iscd_lsc->suggestedWavelengths.GetRangeString_WaveGrid_50GHz().c_str());
                strcat(buf, str);
            }
        }
        list<TSchedule*>::iterator itS = this->schedules.begin();
        if (itS != this->schedules.end())
            strcat(buf, " with feasible scheules: ");
        for (; itS != this->schedules.end(); itS++)
        {
            snprintf(str, 256, " [start:%d-duration:%d] ", (int)(*itS)->GetStartTime(), (*itS)->GetDuration());
            strcat(buf, str);
        }
    }
    strcat(buf, "\n");
    LOG_DEBUG(buf);
}


void TEWG::AddResvDeltas(TReservation* resv)
{
    list<TDelta*>& resvDeltas = resv->GetDeltas();
    list<TLink*>::iterator itl = this->tLinks.begin();
    list<TDelta*>::iterator itd;
    for (; itl != this->tLinks.end(); itl++)
    {
        for (itd = resvDeltas.begin(); itd != resvDeltas.end(); itd++)
        {
            TDelta* delta = *itd;
            if (*(*itl) == (*(TLink*)delta->GetTargetResource()))
            {
                list<TDelta*> oldDeltaList = (*itl)->LookupDeltasByName(resv->GetName());
                if (oldDeltaList.size() == 0)
                {
                    delta = delta->Clone();
                    this->deltaList.push_back(delta);
                    (*itl)->AddDelta(delta);
                } 
                else
                {
                    struct timeval lastGenTime = oldDeltaList.back()->GetGeneratedTime();
                    struct timeval thisGenTime =  delta->GetGeneratedTime();
                    if (lastGenTime < thisGenTime)
                    {
                        // TODO: remove deltas in oldDeltaList from (*itl)->deltaList
                        delta = delta->Clone();
                        this->deltaList.push_back(delta);
                        (*itl)->AddDelta(delta);
                    }
                }
            }
        }
    }
}

void TEWG::RemoveResvDeltas(string& resvName)
{
    //remove and delete deltas from deltaList
    list<TDelta*>::iterator itd = this->deltaList.begin();
    for (; itd != this->deltaList.end(); itd++)
    {
        TDelta* delta = *itd;
        if (delta->GetReservationName() == resvName)
        {
            if (delta->GetTargetResource())
                delta->GetTargetResource()->RemoveDelta(delta);
            delete delta;
            itd = this->deltaList.erase(itd);
        }
    }
}

void TEWG::HoldResvDeltas(string& resvName, bool doHold)
{
    list<TDelta*>::iterator itd = this->deltaList.begin();
    for (; itd != this->deltaList.end(); itd++)
    {
        TDelta* delta = *itd;
        if (delta->GetReservationName() == resvName)
        {
            if (delta->GetTargetResource())
            {
                if (doHold)
                {
                    delta->GetTargetResource()->RemoveDelta(delta);
                    delta->Revoke();
                }
                else 
                {
                    list<TDelta*>::iterator itd = delta->GetTargetResource()->GetDeltaList().begin();
                    for (; itd != delta->GetTargetResource()->GetDeltaList().end(); itd++)
                    {
                        if (*itd == delta)
                            break;
                    }
                    if (itd == delta->GetTargetResource()->GetDeltaList().end())
                    {
                        delta->GetTargetResource()->AddDelta(delta);
                        delta->Apply();
                    }
                }
            }
        }
    }
}

void TEWG::ApplyResvDeltas(string& resvName)
{
    list<TDelta*>::iterator itd = this->deltaList.begin();
    for (; itd != this->deltaList.end(); itd++)
    {
        TDelta* delta = *itd;
        if (delta->GetReservationName() == resvName)
            delta->Apply();
    }
}

void TEWG::RevokeResvDeltas(string& resvName)
{
    list<TDelta*>::iterator itd = this->deltaList.begin();
    for (; itd != this->deltaList.end(); itd++)
    {
        TDelta* delta = *itd;
        if (delta->GetReservationName() == resvName)
            delta->Revoke();
    }
}

void TEWG::PruneByBandwidth(u_int64_t bw)
{
    list<TLink*>::iterator itl = tLinks.begin();
    itl = tLinks.begin();
    while (itl != tLinks.end())
    {
        TLink* L = *itl;
        ++itl;
        if (L->GetAvailableBandwidth() < bw)
            RemoveLink(L);
    }
}

void TEWG::PruneByExclusionUrn(string& exclusionUrn)
{
    list<TLink*>::iterator itl = tLinks.begin();
    itl = tLinks.begin();
    while (itl != tLinks.end())
    {
        TLink* L = *itl;
        ++itl;
        if (L->VerifyContainUrn(exclusionUrn)) 
        {
            RemoveLink(L);
            if (L->GetRemoteLink() != NULL)
                RemoveLink((TLink*)L->GetRemoteLink());
        }
    }
}

list<TLink*> TEWG::ComputeDijkstraPath(TNode* srcNode, TNode* dstNode, bool cleanStart)
{
    // init TWorkData in TLinks
    list<TNode*>::iterator itn = tNodes.begin();
    while (itn != tNodes.end())
    {
        TNode* node = *itn;
        if (node->GetWorkData())
        {
            if (cleanStart) 
                TWDATA(node)->Cleanup();
        }
        else
            node->SetWorkData(new TWorkData());
        ++itn;
    }
    list<TLink*>::iterator itl = tLinks.begin();
    itl = tLinks.begin();
    while (itl != tLinks.end())
    {
        TLink* link = *itl;
        if (link->GetWorkData())
        {
            if (cleanStart) 
                TWDATA(link)->Cleanup();
            TWDATA(link)->linkCost = link->GetMetric();
        }
        else
            link->SetWorkData(new TWorkData(link->GetMetric(), _INF_));
        ++itl;
    }

    // Dijkstra's SPF algorithm
    TWDATA(srcNode)->visited = true;
    TNode* headnode= NULL;
    vector<TNode*> ReachableNodes;
    list<TLink*>::iterator itLink;
    TNode* nextnode;

    itLink = srcNode->GetLocalLinks().begin();
    while (itLink != srcNode->GetLocalLinks().end()) 
    {
        if (!TWDATA(*itLink) || TWDATA(*itLink)->filteroff)
        {
            itLink++;
            continue;
        }
        nextnode=(*itLink)->GetRemoteEnd();
        if (!nextnode || !TWDATA(nextnode) || TWDATA(nextnode)->filteroff)
        {
            itLink++;
            continue;
        }
        ReachableNodes.push_back(nextnode); // found path to this node
        if (TWDATA(nextnode)->pathCost > TWDATA(*itLink)->linkCost)
        {
            TWDATA(nextnode)->pathCost = TWDATA(*itLink)->linkCost;
            TWDATA(nextnode)->path.push_back(*itLink);
        }
        itLink++;
    }
    if (ReachableNodes.size()==0) 
        throw TCEException((char*)"TEWG::ComputeDijkstraPath(): No Path Found");

    vector<TNode*>::iterator start = ReachableNodes.begin();
    vector<TNode*>::iterator end = ReachableNodes.begin();
    start++;
    while (start != ReachableNodes.end())
    {
        if ( TWDATA(*end)->pathCost > TWDATA(*start)->pathCost)
            end = start;
        start++;
    }
    headnode = *end;
    TWDATA(headnode)->visited = true;
    ReachableNodes.erase(end);

    for (;;) 
    {
        if (headnode==NULL) 
            break;
    	// Go through all the outgoing links for the newly added node
    	itLink = headnode->GetLocalLinks().begin();
        while (itLink!=headnode->GetLocalLinks().end()) 
        {
            nextnode=(*itLink)->GetRemoteEnd();
            if (nextnode && TWDATA(nextnode) && !TWDATA(nextnode)->visited && !TWDATA(nextnode)->filteroff && !TWDATA(*itLink)->filteroff 
                && TWDATA(nextnode)->pathCost > TWDATA(headnode)->pathCost + TWDATA(*itLink)->linkCost)
            {
                TWDATA(nextnode)->pathCost = TWDATA(headnode)->pathCost + TWDATA(*itLink)->linkCost;
                bool hasNode = false;
                vector<TNode*>::iterator itRNode;
                itRNode=ReachableNodes.begin();
                while ((!hasNode) && (itRNode!=ReachableNodes.end())) 
                {
                    if ((*itRNode)==nextnode) 
                        hasNode = true;
                    itRNode++;
                }
                if (!hasNode)
                {
                    ReachableNodes.push_back(nextnode);
                    //LOG("DijkstraPath can reach node: " << nextnode->GetName() <<endl);
                }
                TWDATA(nextnode)->path.assign(TWDATA(headnode)->path.begin(), TWDATA(headnode)->path.end());
                TWDATA(nextnode)->path.push_back(*itLink);
            }
            itLink++;
        }

        if (ReachableNodes.size()==0) 
            break;
        start = ReachableNodes.begin();
        end = ReachableNodes.begin();
        start++;
        while (start != ReachableNodes.end())
        {
            if ( TWDATA(*end)->pathCost > TWDATA(*start)->pathCost)
                end = start;
            start++;
        }
        headnode= *end;
        TWDATA(headnode)->visited=true;
        ReachableNodes.erase(end);
        if (headnode == dstNode)
        {
            //TPath tPath;
            //tPath.SetPath(TWDATA(dstNode)->path);
            //LOG("DijkstraPath return "<<endl);
            //tPath.LogDump();
            return TWDATA(dstNode)->path;
        }
    } 
    // no path found
    throw TCEException((char*)"TEWG::ComputeDijkstraPath(): No Path Found");
}


// An implementation of Yen's KSP algorithm. --> To be moved to TEWG class as common method.
void TEWG::ComputeKShortestPaths(TNode* srcNode, TNode* dstNode, int K, vector<TPath*>& KSP)
{
    int KSPcounter=0;    
    KSP.clear();
    vector<TPath*> CandidatePaths;
    list<TLink*>::iterator itLink;
    list<TLink*>::iterator pathstart;
    list<TLink*>::iterator pathend;

    try {
        this->ComputeDijkstraPath(srcNode, dstNode, true);
    } catch (TCEException e) {
        throw TCEException((char*)"TEWG::ComputeKShortestPaths() Initial call of ComputeDijkstraPath found no path.");
    }    

    TPath* nextpath = new TPath();
    nextpath->GetPath().assign(TWDATA(dstNode)->path.begin(), TWDATA(dstNode)->path.end());
    nextpath->SetCost(TWDATA(dstNode)->pathCost);
    nextpath->SetDeviationNode(srcNode);
    CandidatePaths.push_back(nextpath);
    KSP.push_back(nextpath);
    KSPcounter++;

    vector<TPath*>::iterator itPath;
    while ((CandidatePaths.size() > 0) && (KSPcounter <= K))
    {
        // reset graph TWData but keep filter
        list<TNode*>::iterator itNode;
        bool oldFilter;
        for (itNode = tNodes.begin(); itNode != tNodes.end(); itNode++)
        {
            if (!TWDATA(*itNode))
                continue;
            oldFilter = TWDATA(*itNode)->filteroff;
            TWDATA(*itNode)->Cleanup();
            TWDATA(*itNode)->filteroff = oldFilter;
        }
        for (itLink = tLinks.begin(); itLink != tLinks.end(); itLink++)
        {
            if (!TWDATA(*itLink))
                continue;
            oldFilter = TWDATA(*itLink)->filteroff;
            TWDATA(*itLink)->Cleanup();
            TWDATA(*itLink)->filteroff = oldFilter;
        }
        // get so-far least cost candidate path
        itPath = min_element(CandidatePaths.begin(), CandidatePaths.end(), cmp_tpath);
        TPath* headpath= *itPath;
        CandidatePaths.erase(itPath); 
        //headpath->LogDump(); 
        if (KSPcounter > 1) 
            KSP.push_back(headpath);
        if (KSPcounter == K) 
            break;

        itLink = headpath->GetPath().begin();
        while ((*itLink)->GetLocalEnd() != headpath->GetDeviationNode()) 
        {
            //filteroff for local end of the link
            list<TLink*>::iterator itl;
            for (itl = (*itLink)->GetLocalEnd()->GetLocalLinks().begin(); itl != (*itLink)->GetLocalEnd()->GetLocalLinks().end(); itl++)
                TWDATA(*itl)->filteroff = true; 
            for (itl = (*itLink)->GetLocalEnd()->GetRemoteLinks().begin(); itl != (*itLink)->GetLocalEnd()->GetRemoteLinks().end(); itl++)
                TWDATA(*itl)->filteroff = true; 
            TWDATA((*itLink)->GetLocalEnd())->filteroff = true; 
            
            itLink++;
        }
        pathend = headpath->GetPath().end();
        // penultimate node along path p_k (segment starting from deviationNode)
        for ( ; itLink != pathend; itLink++) 
        {
            // filteroff all masked links for headpath before new search
            headpath->FilterOffMaskedLinks(true);
            TWDATA(*itLink)->filteroff = true;
            // again, eset graph TWData but keep filter
            list<TNode*>::iterator itNode2;
            list<TLink*>::iterator itLink2;
            for (itNode2 = tNodes.begin(); itNode2 != tNodes.end(); itNode2++)
            {
                if (!TWDATA(*itNode2))
                    continue;
                oldFilter = TWDATA(*itNode2)->filteroff;
                TWDATA(*itNode2)->Cleanup();
                TWDATA(*itNode2)->filteroff = oldFilter;
            }
            for (itLink2 = tLinks.begin(); itLink2 != tLinks.end(); itLink2++)
            {
                if (!TWDATA(*itLink2))
                    continue;
                oldFilter = TWDATA(*itLink2)->filteroff;
                TWDATA(*itLink2)->Cleanup();
                TWDATA(*itLink2)->filteroff = oldFilter;
            }
            // compute shortest path from local end of current link
            //LogDumpWithFlags();
            try {
                this->ComputeDijkstraPath((*itLink)->GetLocalEnd(), dstNode);
            } catch (TCEException e) {
                ; // do nothing
            }    
            // find SPF from Vk_i to destination node
            if (TWDATA(dstNode)->path.size()>0) 
            {
                // concatenate subpk(s, vk_i) to shortest path found from vk_i to destination
                nextpath = new TPath(); // $$ Memory leak...
                if (itLink != headpath->GetPath().begin()) 
                {
                    nextpath->GetPath().assign(headpath->GetPath().begin(), itLink);
                    nextpath->SetDeviationNode((*itLink)->GetLocalEnd());
                } 
                else 
                {
                    nextpath->SetDeviationNode(srcNode);
                } 
                list<TLink*>::iterator halfpath;
                halfpath = TWDATA(dstNode)->path.begin();
                while (halfpath!= TWDATA(dstNode)->path.end())
                {
                    (nextpath->GetPath()).push_back(*halfpath);
                    halfpath++;
                }

                // calculate the path cost
                nextpath->CalculatePathCost(); 
                // mask the parent path and current path (links have filteroff == true)
                nextpath->GetMaskedLinkList().assign(headpath->GetMaskedLinkList().begin(),headpath->GetMaskedLinkList().end());
                nextpath->GetMaskedLinkList().push_back(*itLink);
                //$$ nextpath->DisplayPath();
                CandidatePaths.push_back(nextpath);
            }
            // reset filteroff for current link
            TWDATA(*itLink)->filteroff = false;            
        }
        KSPcounter++;
    }
    // release memory of remaining paths in CandidatePaths list
    /*
    for (int i = 0; i < CandidatePaths.size(); i++)
    {
        delete CandidatePaths[i];
    }
    */
}



void TEWG::LogDumpWithFlags()
{
    char buf[102400]; //up to 100K
    char str[256];
    strcpy(buf, "TEWG Dump...\n");
    list<TDomain*>::iterator itd = this->tDomains.begin();
    for (; itd != this->tDomains.end(); itd++)
    {
        TDomain* td = (*itd);
        snprintf(str, 256, "<domain id=%s>\n", td->GetName().c_str());
        strcat(buf, str);
        map<string, Node*, strcmpless>::iterator itn = td->GetNodes().begin();
        for (; itn != td->GetNodes().end(); itn++)
        {
            TNode* tn = (TNode*)(*itn).second;
            snprintf(str, 128, "\t<node id=%s>\n", tn->GetName().c_str());
            strcat(buf, str);
            if (tn->GetWorkData())
            {
                snprintf(str, 128, "\t\t<visited=%d> <filteroff=%d>\n",
                    TWDATA(tn)->visited, TWDATA(tn)->filteroff);
                strcat(buf, str);
            }
            map<string, Port*, strcmpless>::iterator itp = tn->GetPorts().begin();
            for (; itp != tn->GetPorts().end(); itp++)
            {
                TPort* tp = (TPort*)(*itp).second;
                snprintf(str, 128, "\t\t<port id=%s>\n", tp->GetName().c_str());
                strcat(buf, str);
                map<string, Link*, strcmpless>::iterator itl = tp->GetLinks().begin();
                for (; itl != tp->GetLinks().end(); itl++) 
                {
                    TLink* tl = (TLink*)(*itl).second;
                    snprintf(str, 128, "\t\t\t<link id=%s>\n", tl->GetName().c_str());
                    strcat(buf, str);
                    if (tl->GetWorkData())
                    {
                        snprintf(str, 128, "\t\t\t\t<visited=%d> <filteroff=%d>\n",  
                            TWDATA(tl)->visited, TWDATA(tl)->filteroff);
                        strcat(buf, str);
                    }
                    if (tl->GetRemoteLink())
                    {
                        snprintf(str, 128, "\t\t\t\t<remoteLinkId>domain=%s:node=%s:port=%s:link=%s</remoteLinkId>\n",  
                            tl->GetRemoteLink()->GetPort()->GetNode()->GetDomain()->GetName().c_str(),
                            tl->GetRemoteLink()->GetPort()->GetNode()->GetName().c_str(),
                            tl->GetRemoteLink()->GetPort()->GetName().c_str(), 
                            tl->GetRemoteLink()->GetName().c_str());
                        strcat(buf, str);
                    }
                    snprintf(str, 128, "\t\t\t</link>\n");
                    strcat(buf, str);
                }
                snprintf(str, 128, "\t\t</port>\n");
                strcat(buf, str);
            }
            snprintf(str, 128, "\t</node>\n");
            strcat(buf, str);
        }
        snprintf(str, 128, "</domain>\n");
        strcat(buf, str);
    }    
    LOG_DEBUG(buf);
}

