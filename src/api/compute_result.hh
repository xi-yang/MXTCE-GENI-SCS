/*
 * compute_result.hh
 *
 *  Created on: May 18, 2011
 *      Author: wind
 */

#ifndef COMPUTE_RESULT_HH_
#define COMPUTE_RESULT_HH_

#include "resource.hh"
#include <vector>


using namespace std;

class Link_info
{
protected:
	string linkName;
	int capacity;
	int mtu;
	bool vlanTranslation;
	string switchingType;
	string encodingType;
	string assignedVlanTags;
	string suggestedVlanTags;
	string availableVlanTags;

public:
	void setLinkName(string& t_linkName) {this->linkName = t_linkName;}
	string& getLinkName() {return this->linkName;}
	void setCapacity(int t_capacity) {this->capacity = t_capacity;}
	int getCapacity() {return this->capacity;}
	void setMtu(int t_mtu) {this->mtu = t_mtu;}
	int getMtu() {return this->mtu;}
	void setVlanTranslation(bool t_vlanTranslation) {this->vlanTranslation = t_vlanTranslation;}
	bool getVlanTranslation() {return this->vlanTranslation;}
	void setSwitchingType(string& t_switchingType) {this->switchingType = t_switchingType;}
	string& getSwitchingType() {return this->switchingType;}
	void setEncodingType(string& t_encodingType) {this->encodingType = t_encodingType;}
	string& getEncodingType() {return this->encodingType;}
	void setAssignedVlanTags(string& t_assignedVlanTags) {this->assignedVlanTags = t_assignedVlanTags;}
	string& getAssignedVlanTags() {return this->assignedVlanTags;}
	void setSuggestedVlanTags(string& t_suggestedVlanTags) {this->suggestedVlanTags = t_suggestedVlanTags;}
	string& getSuggestedVlanTags() {return this->suggestedVlanTags;}
	void setAvailableVlanTags(string& t_availableVlanTags) {this->availableVlanTags = t_availableVlanTags;}
	string& getAvailableVlanTags() {return this->availableVlanTags;}


};

class Compute_result
{
protected:
	string gri;
	list<Link_info*> path;

public:
	void setGri(string& t_gri) {this->gri = t_gri;}
	string& getGri() {return this->gri;}
    list<Link_info*>& GetPath() { return path; }
    void SetPath(list<Link_info*>& t_path) { path.assign(t_path.begin(), t_path.end()); }

};


#endif /* COMPUTE_RESULT_HH_ */
