/*
 * stornet_constraint.hh
 *
 *  Created on: Jul 19, 2011
 *      Author: dell
 */

#ifndef _STORNET_CONSTRAINT_HH_
#define _STORNET_CONSTRAINT_HH_

#include "optional_constraint.hh"

class Apimsg_stornet_constraint:public Apimsg_opti_constraint
{
protected:
	string co_schedule_request_id;
	u_int32_t start_time;
	u_int32_t end_time;
	u_int32_t min_bandwidth;
	u_int32_t max_num_of_alt_paths;
	bool bandwidth_avai_graph;
	bool contiguous_vlan;
	u_int32_t max_duration;
	u_int32_t max_bandwidth;
	u_int32_t data_size_bytes;

public:
	Apimsg_stornet_constraint()
	{
		this->initVar();
	}

	virtual ~Apimsg_stornet_constraint(){}

	void initVar()
	{
		this->co_schedule_request_id = "";
		this->start_time = 0;
		this->end_time = 0;
		this->min_bandwidth = 0;
		this->max_num_of_alt_paths = 0;
		this->bandwidth_avai_graph = false;
		this->contiguous_vlan = false;
		this->max_bandwidth = 0;
		this->max_duration = 0;
		this->data_size_bytes = 0;

	}

	void setCoschedulerequestid(string t_co_schedule_request_id) {this->co_schedule_request_id = t_co_schedule_request_id;}
	string& getCoschedulerequestid() {return this->co_schedule_request_id;}

	void setStarttime(u_int32_t t_start_time) {this->start_time = t_start_time;}
	u_int32_t getStarttime() {return this->start_time;}

	void setEndtime(u_int32_t t_end_time) {this->end_time = t_end_time;}
	u_int32_t getEndtime() {return this->end_time;}

	void setMinbandwidth(u_int32_t t_min_bandwidth) {this->min_bandwidth = t_min_bandwidth;}
	u_int32_t getMinbandwidth() {return this->min_bandwidth;}

	void setMaxnumofaltpaths(u_int32_t t_max_num_of_alt_paths) {this->max_num_of_alt_paths = t_max_num_of_alt_paths;}
	u_int32_t getMaxnumofaltpaths() {return this->max_num_of_alt_paths;}

	void setBandwidthavaigraph(bool t_bandwidth_avai_graph) {this->bandwidth_avai_graph = t_bandwidth_avai_graph;}
	bool getBandwidthavaigraph() {return this->bandwidth_avai_graph;}

	void setContiguousvlan(bool t_contiguous_vlan) {this->contiguous_vlan = t_contiguous_vlan;}
	bool getContiguousvlan() {return this->contiguous_vlan;}

	void setMaxduration(u_int32_t t_max_duration) {this->max_duration = t_max_duration;}
	u_int32_t getMaxduration() {return this->max_duration;}

	void setMaxbandwidth(u_int32_t t_max_bandwidth) {this->max_bandwidth = t_max_bandwidth;}
	u_int32_t getMaxbandwidth() {return this->max_bandwidth;}

	void setDatasizebytes(u_int32_t t_data_size_bytes) {this->data_size_bytes = t_data_size_bytes;}
	u_int32_t getDatasizebytes() {return this->data_size_bytes;}




};



#endif /* STORNET_CONSTRAINT_HH_ */
