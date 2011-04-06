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
    for (int i = 0; i < 8; i++)
        tl->unreservedBandwidth[i] = this->unreservedBandwidth[i];
    list<ISCD*>::iterator its = swCapDescriptors.begin();
    for (; its != swCapDescriptors.end(); its++)
        tl->swCapDescriptors.push_back((*its)->Duplicate());
    list<IACD*>::iterator ita = swAdaptDescriptors.begin();
    for (; ita != swAdaptDescriptors.end(); ita++)
        tl->swAdaptDescriptors.push_back(*ita);
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
               if (((ISCD_TDM*)iscd)->minReservableBandwidth == 0 || tspec.Bandwidth % ((ISCD_TDM*)iscd)->minReservableBandwidth == 0)
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
                if (((ISCD_TDM*)iscd)->minReservableBandwidth == 0 || tspec.Bandwidth % ((ISCD_TDM*)iscd)->minReservableBandwidth == 0)
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
void TLink::ProceedByUpdatingWaves(ConstraintTagSet &head_waveset, ConstraintTagSet &next_waveset)
{
    next_waveset.Clear();
    bool any_wave_ok = head_waveset.HasAnyTag();

    // TODO: vendor specific wavelength constraint handling
    //load up next_wavset from link

    if (!any_wave_ok)
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


bool TLink::CrossingRegionBoundary(TSpec& tspec)
{
    // Check adaptation defined by IACD(s)
    list<IACD*>::iterator it_iacd;
    for (it_iacd = swAdaptDescriptors.begin(); it_iacd != swAdaptDescriptors.end(); it_iacd++)
    {
        //crossing from lower layer to upper layer
        if ((*it_iacd)->lowerLayerSwitchingType == tspec.SWtype && (*it_iacd)->lowerLayerEncodingType == tspec.ENCtype)
            return true;
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

bool TLink::GetNextRegionTspec(TSpec& tspec)
{
    // Check adaptation defined by IACD(s)
    list<IACD*>::iterator it_iacd;
    for (it_iacd = swAdaptDescriptors.begin(); it_iacd != swAdaptDescriptors.end(); it_iacd++)
    {
        //crossing from lower layer to upper layer
        if ((*it_iacd)->lowerLayerSwitchingType == tspec.SWtype && (*it_iacd)->lowerLayerEncodingType == tspec.ENCtype)
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
        
        //crossing from upper layer to lower layer
        if ((*it_iacd)->upperLayerSwitchingType == tspec.SWtype && (*it_iacd)->upperLayerSwitchingType == tspec.ENCtype)
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


    // Check implicit adaptation
    if (this->remoteLink)
    {
        ISCD* iscd_adapted = NULL;         
        if (this->swAdaptDescriptors.size() == 1 && remoteLink->GetSwAdaptDescriptors().size() == 1 && this->GetTheISCD()->switchingType != ((TLink*)remoteLink)->GetTheISCD()->switchingType)
            iscd_adapted = ((TLink*)remoteLink)->GetTheISCD();
        else if (this->swAdaptDescriptors.size()*remoteLink->GetSwAdaptDescriptors().size() > 1)
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
    list<TLink*>::iterator itL;
    TLink* lastLink = NULL;
    for (itL = path.begin(); itL != path.end(); itL++)
    {
        TLink* link = *itL;
        string urn = link->GetName();
        ParseFQUrn(urn, domainName, nodeName, portName, linkName);
        assert(domainName.length() > 0 && nodeName.length() > 0 && portName.length() > 0 && linkName.length() > 0);
        assert(LookupLinkByURN(urn) == NULL && LookupPortByURN(urn) == NULL);
        link->SetName(linkName);
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
    char buf[102400]; //up to 100K
    char str[128];
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
                    if (tl->GetRemoteLink())
                    {
                        snprintf(str, 128, "\t\t\t\t<remoteLinkId>domain=%s:node=%s:port=%s:link=%s</remoteLinkId>\n",  
                            tl->GetRemoteLink()->GetPort()->GetNode()->GetDomain()->GetName().c_str(),
                            tl->GetRemoteLink()->GetPort()->GetNode()->GetName().c_str(),
                            tl->GetRemoteLink()->GetPort()->GetName().c_str(), 
                            tl->GetRemoteLink()->GetName().c_str());
                        strcat(buf, str);
                    }
                    if (tl->GetTheISCD())
                    {
                        snprintf(str, 128, "\t\t\t\t<SwitchingCapabilityDescriptors> <switchingcapType=%d><encodingType=%d><capacity=%ld> </SwitchingCapabilityDescriptors>\n",  
                            tl->GetTheISCD()->switchingType,
                            tl->GetTheISCD()->encodingType,
                            tl->GetTheISCD()->capacity);
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
}


// verify constrains of vlantag, wavelength and cross-layer adapation (via Tspec) 
bool TPath::VerifyTEConstraints(u_int32_t& srcVtag, u_int32_t& dstVtag, u_int32_t& wave, TSpec& tspec) 
{
    TLink* L;
    list<TLink*>::iterator iterL;
    ConstraintTagSet head_vtagset(MAX_VLAN_NUM), next_vtagset(MAX_VLAN_NUM);
    ConstraintTagSet head_vtagset_trans(MAX_VLAN_NUM), next_vtagset_trans(MAX_VLAN_NUM);
    ConstraintTagSet init_vtagset(MAX_VLAN_NUM);
    bool no_vtag = false, no_vtag_trans = false;
    ConstraintTagSet head_waveset(MAX_WAVE_NUM, 190000, 100), next_waveset(MAX_WAVE_NUM, 190000, 100);

    if (path.size() == 0)
        return false;

    // initializing tspecs
    for (iterL = path.begin(); iterL != path.end(); iterL++)
    {
        L = *iterL;
        if (L->GetLocalEnd())
            TWDATA(L->GetLocalEnd())->tspec.Update(tspec.SWtype, tspec.ENCtype, tspec.Bandwidth);
        if (L->GetRemoteEnd())
            TWDATA(L->GetRemoteEnd())->tspec.Update(tspec.SWtype, tspec.ENCtype, tspec.Bandwidth);
        TWDATA(L)->tspec.Update(tspec.SWtype, tspec.ENCtype, tspec.Bandwidth);
    }

    // initializing vtags and wavelengths
    if (srcVtag != 0)
        head_vtagset.AddTag(srcVtag);
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
        if (!head_waveset.IsEmpty())
        {
            L->ProceedByUpdatingWaves(head_waveset, next_waveset);
            if (next_waveset.IsEmpty())
                return false;
            head_waveset = next_waveset;
        }

        if (L->CrossingRegionBoundary(TWDATA(L->GetLocalEnd())->tspec))
        {
            L->GetNextRegionTspec(TWDATA(L->GetRemoteEnd())->tspec);
            // TODO: WDM  special handling
            /*
            if (has_wdm_layer && (L->rmt_end->tspec.SWtype == LINK_IFSWCAP_SUBTLV_SWCAP_LSC || L->rmt_end->tspec.SWtype == MOVAZ_LSC))
            {
            }
            else if (has_wdm_layer && (L->lcl_end->tspec.SWtype == LINK_IFSWCAP_SUBTLV_SWCAP_LSC || L->lcl_end->tspec.SWtype == MOVAZ_LSC))
            {
                head_waveset.Clear();
            }
            */
        }       
        else
        {
            TWDATA(L->GetRemoteEnd())->tspec = TWDATA(L->GetLocalEnd())->tspec;
        }
    }

    // pick src and dst vtags from two sets of head_set and next_set (randomized by default)
    // try using common (continuous) vlan tag if possible, otherwise (no_vtag==true) use translated diff vlan
    if (no_vtag)
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
    // pick wavelength (randomized)
    wave = next_waveset.RandomTag();
    return true;
}

// TODO: vlanRange instead of single tags ? 
void TPath::UpdateLayer2Info(u_int32_t srcVtag, u_int32_t dstVtag)
{
    TLink* L;
    list<TLink*>::iterator iterL;
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
        iscd->suggestedVlanTags.Clear();
        if (forwardContinued && iscd->availableVlanTags.HasTag(srcVtag))
            iscd->suggestedVlanTags.AddTag(srcVtag);
        else 
            forwardContinued = false;
    }
    if (dstVtag == srcVtag)
        return;
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
        if (iscd->suggestedVlanTags.HasTag(srcVtag) || iscd->availableVlanTags.HasTag(dstVtag))
            break;
    }        
}

TPath* TPath::Clone()
{
    TPath* P = new TPath;
    P->SetCost(this->cost);
    P->SetIndependent(this->independent);
    list<TLink*>::iterator itL;
    TLink* lastLink = NULL;
    for (itL = this->path.begin(); itL != this->path.end(); itL++)
    {
        TLink* L = *itL;
        P->GetPath().push_back((L)->Clone());
        if (lastLink && L->GetPort()->GetNode() != lastLink->GetPort()->GetNode())
        {
            lastLink->SetRemoteLink(L);
            L->SetRemoteLink(lastLink);
        }
        lastLink = L;
    }
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
            if (L->GetRemoteLink())
            {
                snprintf(str, 256, " ->%s:%s:%s:%s",
                    L->GetRemoteLink()->GetPort()->GetNode()->GetDomain()->GetName().c_str(),
                    L->GetRemoteLink()->GetPort()->GetNode()->GetName().c_str(),
                    L->GetRemoteLink()->GetPort()->GetName().c_str(), 
                    L->GetRemoteLink()->GetName().c_str());
                strcat(buf, str);
            }
            list<ISCD*>::iterator it;
            ISCD_L2SC* iscd = NULL;
            for (it = L->GetSwCapDescriptors().begin(); it !=  L->GetSwCapDescriptors().end(); it++)
            {
                if ((*it)->switchingType != LINK_IFSWCAP_L2SC || (*it)->encodingType != LINK_IFSWCAP_ENC_ETH)
                    continue;
                iscd = (ISCD_L2SC*)(*it);
                break;
            }
            if (iscd)
            {
                snprintf(str, 256, " (suggestedVlan:%s) ", iscd->suggestedVlanTags.GetRangeString().c_str());
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

void TEWG::PruneByBandwidth(long bw)
{
    list<TLink*>::iterator itl = tLinks.begin();
    itl = tLinks.begin();
    while (itl != tLinks.end())
    {
        TLink* link = *itl;
        ++itl;
        if (link->GetAvailableBandwidth() < bw)
            RemoveLink(link);
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
        this->ComputeDijkstraPath(srcNode, dstNode);
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
    char str[128];
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

