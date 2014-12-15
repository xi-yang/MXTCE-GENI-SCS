#include "request_encoder.hh"
#include <fstream>
#include <stdlib.h>


int Apireqmsg_encoder::msg_seq_num=0;

api_msg* Apireqmsg_encoder::test_encode_msg(Apimsg_user_constraint* user_cons)
{
	int msg_sublen=0;
	int msg_sub_startlen=0;
	int msg_len=0;
	int msg_header_size=0;
	int total_size=0;
	u_int8_t* msg_sub_ptr;
	u_int8_t* msg_sub_start_ptr;
	u_int8_t* msg_body_ptr;
	u_int8_t* msg_ptr;
	u_int8_t pceType=PCE_USERCONSTRAINT;

	string gri="";
	u_int32_t start_time=0;
	u_int32_t end_time=0;
	u_int32_t bandwidth=0;
	string layer="";
	string path_setup_model="";
	string path_type="";
	string src_vlan_tag="";
	string dest_vlan_tag="";
	string src_end_point="";
	string dest_end_point="";
	u_int32_t src_ip_port=0;
	u_int32_t dest_ip_port=0;
	string protocol="";
	string dscp="";
	u_int32_t burst_limit=0;
	string lsp_class="";


	gri = user_cons->getGri();
	start_time = user_cons->getStarttime();
	end_time = user_cons->getEndtime();
	bandwidth = user_cons->getBandwidth();
	layer = user_cons->getLayer();
	path_setup_model = user_cons->getPathsetupmodel();
	path_type = user_cons->getPathtype();
	src_end_point = user_cons->getSrcendpoint();
	dest_end_point = user_cons->getDestendpoint();

	if(layer=="2")
	{
		src_vlan_tag = user_cons->getSrcvlantag();
		dest_vlan_tag = user_cons->getDestvlantag();
	}
	else if(layer=="3")
	{
		src_ip_port = user_cons->getSrcipport();
		dest_ip_port = user_cons->getDestipport();
		protocol = user_cons->getProtocol();
		dscp = user_cons->getDscp();
		burst_limit = user_cons->getBurstlimit();
		lsp_class = user_cons->getLspclass();
	}

	if(gri!="")
	{
		pri_type_encoder->encodeString(PCE_GRI, gri);
	}

	if(start_time!=0)
	{
		pri_type_encoder->encodeInteger(PCE_STARTTIME, (int)start_time);
	}

	if(end_time!=0)
	{
		pri_type_encoder->encodeInteger(PCE_ENDTIME, (int)end_time);
	}

	if(bandwidth!=0)
	{
		pri_type_encoder->encodeInteger(PCE_BANDWIDTH, (int)bandwidth);
	}

	if(src_end_point!="")
	{
		pri_type_encoder->encodeString(PCE_SOURCE, src_end_point);
	}

	if(dest_end_point!="")
	{
		pri_type_encoder->encodeString(PCE_DESTINATION, dest_end_point);
	}

	if(path_setup_model!="")
	{
		pri_type_encoder->encodeString(PCE_PATHSETUPMODEL, path_setup_model);
	}

	if(path_type!="")
	{
		pri_type_encoder->encodeString(PCE_PATHTYPE, path_type);
	}

	if(layer!="")
	{
		pri_type_encoder->encodeString(PCE_LAYER, layer);
	}

	if(layer=="2")
	{
		if(src_vlan_tag!="")
		{
			pri_type_encoder->encodeString(PCE_SRCVLAN, src_vlan_tag);
		}

		if(dest_vlan_tag!="")
		{
			pri_type_encoder->encodeString(PCE_DESTVLAN, dest_vlan_tag);
		}
	}
	else if(layer=="3")
	{
		if(src_ip_port!=0)
		{
			pri_type_encoder->encodeInteger(PCE_SRCIPPORT, src_ip_port);
		}

		if(dest_ip_port!=0)
		{
			pri_type_encoder->encodeInteger(PCE_DESTIPPORT, dest_ip_port);
		}

		if(protocol!="")
		{
			pri_type_encoder->encodeString(PCE_L3_PROTOCOL, protocol);
		}

		if(dscp!="")
		{
			pri_type_encoder->encodeString(PCE_L3_DSCP, dscp);
		}

		if(burst_limit!=0)
		{
			pri_type_encoder->encodeInteger(PCE_MPLS_BURSTLIMIT, burst_limit);
		}

		if(lsp_class!="")
		{
			pri_type_encoder->encodeString(PCE_MPLS_LSPCLASS, lsp_class);
		}

	}
	else
	{
        return NULL;
	}

	msg_sub_ptr=pri_type_encoder->get_buff();
	msg_sublen=pri_type_encoder->get_length();
	msg_sub_start_ptr=encode_msg_sub_start(pceType, msg_sublen, msg_sub_startlen);
	msg_body_ptr=encode_merge_buff(msg_sub_start_ptr, msg_sub_startlen, msg_sub_ptr, msg_sublen);
	msg_len=msg_sub_startlen + msg_sublen; //msg_body_len
	delete[] msg_sub_start_ptr;
	delete[] msg_sub_ptr;
    api_msg* amsg = new api_msg;
	msg_header_size=sizeof(api_msg_header);
	encode_msg_header(&amsg->header, msg_len);
    amsg->body = (char*)msg_body_ptr;
    return amsg;
}

u_int8_t* Apireqmsg_encoder::encode_msg_sub_start(u_int8_t pceType, int msg_sublen, int& sub_start_len)
{
	int size=0;
	int length=msg_sublen;
	int offset=0;
	u_int8_t* header_ptr;

	if(length<0x80)
	{
		size=1;
	}
	else if(length<=0xFF)
	{
		size=2;
	}
	else if(length<=0xFFFF)
	{
		size=3;
	}
	else if(length<=0xFFFFFF)
	{
		size=4;
	}
	else
	{

	}
	size=size+1;
	header_ptr = new u_int8_t[size];
	sub_start_len = size;

	header_ptr[offset++] = pceType;

	if(length<0x80)
	{
		header_ptr[offset++] = (u_int8_t)(length & 0xFf);
	}
	else if(length<=0xFF)
	{
		header_ptr[offset++] = (0x01 | ASN_LONG_LEN);
		header_ptr[offset++] = (u_int8_t)(length & 0xFF);
	}
	else if(length<=0xFFFF)
	{
		header_ptr[offset++] = (0x02 | ASN_LONG_LEN);
		header_ptr[offset++] = (u_int8_t)((length>>8) & 0xFF);
		header_ptr[offset++] = (u_int8_t)(length & 0xFF);
	}
	else if(length<=0xFFFFFF)
	{
		header_ptr[offset++] = (0x03 | ASN_LONG_LEN);
		header_ptr[offset++] = (u_int8_t)((length>>16) & 0xFF);
		header_ptr[offset++] = (u_int8_t)((length>>8) & 0xFF);
		header_ptr[offset++] = (u_int8_t)(length & 0xFF);
	}
	else
	{

	}

	return header_ptr;
}

u_int8_t* Apireqmsg_encoder::encode_merge_buff(u_int8_t* buff1, int len1, u_int8_t* buff2, int len2)
{
	u_int8_t* buff_tmp = new u_int8_t[len1+len2];
	memcpy(buff_tmp, buff1, len1);
	memcpy(buff_tmp+len1, buff2, len2);
	return buff_tmp;
}

void Apireqmsg_encoder::encode_msg_header(api_msg_header* apimsg_header, int msg_body_len)
{
	u_int16_t len=0;
	len = (u_int16_t)msg_body_len;
	apimsg_header->type = htons(API_MSG_REQUEST);
	apimsg_header->length = htons(len);
	apimsg_header->ucid = htonl(0x0002);
	apimsg_header->seqnum = htonl(msg_seq_num);
	apimsg_header->chksum = MSG_CHKSUM(*apimsg_header);
	apimsg_header->options = htonl(0x0000);
	apimsg_header->tag = htonl(0x0000);
	msg_seq_num++;


}

