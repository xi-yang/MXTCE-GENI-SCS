#ifndef _reserved_constraint_hh_
#define _reserved_constraint_hh_

#include "api_constraint.hh"

class Apimsg_resv:public Apimsg_constraint
{
protected:
	string gri;
	u_int32_t start_time;
	u_int32_t end_time;
	u_int32_t bandwidth;
	string layer;
	string path_setup_model;
	string path_type;
	string src_vlan_tag;
	string dest_vlan_tag;
	string src_end_point;
	string dest_end_point;
	u_int32_t src_ip_port;
	u_int32_t dest_ip_port;



public:
	Apimsg_resv(){};
	virtual ~Apimsg_resv(){}

	void setGri(string t_gri) {this->gri = t_gri;}
	string getGri() {return this->gri;}

	void setStarttime(u_int32_t t_start_time) {this->start_time = t_start_time;}
	u_int32_t getStarttime() {return this->start_time;}

	void setEndtime(u_int32_t t_end_time) {this->end_time = t_end_time;}
	u_int32_t getEndtime() {return this->end_time;}

	void setBandwidth(u_int32_t t_bandwidth) {this->bandwidth = t_bandwidth;}
	u_int32_t getBandwidth() {return this->bandwidth;}

	void setLayer(string t_layer) {this->layer = t_layer;}
	string getLayer() {return this->layer;}

	void setPathsetupmodel(string t_path_setup_model) {this->path_setup_model = t_path_setup_model;}
	string getPathsetupmodel() {return this->path_setup_model;}

	void setPathtype(string t_path_type) {this->path_type = t_path_type;}
	string getPathtype() {return this->path_type;}

	void setSrcvlantag(string t_src_vlan_tag) {this->src_vlan_tag = t_src_vlan_tag;}
	string getSrcvlantag() {return this->src_vlan_tag;}

	void setDestvlantag(string t_dest_vlan_tag) {this->dest_vlan_tag = t_dest_vlan_tag;}
	string getDestvlantag() {return this->dest_vlan_tag;}

	void setSrcendpoint(string t_src_end_point) {this->src_end_point = t_src_end_point;}
	string getSrcendpoint() {return this->src_end_point;}

	void setDestendpoint(string t_dest_end_point) {this->dest_end_point = t_dest_end_point;}
	string getDestendpoint() {return this->dest_end_point;}

	void setSrcipport(u_int32_t t_src_ip_port) {this->src_ip_port = t_src_ip_port;}
	u_int32_t getSrcipport() {return this->src_ip_port;}

	void setDestipport(u_int32_t t_dest_ip_port) {this->dest_ip_port = t_dest_ip_port;}
	u_int32_t getDestipport() {return this->dest_ip_port;}

	void setProtocol(string t_protocol) {this->protocol = t_protocol;}
	string getProtocol() {return this->protocol;}

	void setDscp(string t_dscp) {this->dscp = t_dscp;}
	string getDscp() {return this->dscp;}

	void setBurstlimit(u_int32_t t_burst_limit) {this->burst_limit = t_burst_limit;}
	u_int32_t getBurstlimit() {return this->burst_limit;}

	void setLspclass(string t_lsp_class) {this->lsp_class = t_lsp_class;}
	string getLspclass() {return this->lsp_class;}


};

#endif
