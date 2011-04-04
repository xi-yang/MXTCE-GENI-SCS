
#include "request_decoder.hh"
#include <fstream>
#include <stdlib.h>
#include "log.hh"
#include "api.hh"


Apimsg_user_constraint* Apireqmsg_decoder::test_decode_msg(char* apimsg_body, int msg_length)
{
    char* decode_ptr;
    u_int8_t type;
    int len_tag_len;
    Apimsg_user_constraint* user_cons=NULL;
    int length=0;
    int length_decoded=0;

    char buf[500];
    // snprintf(buf, 128, "length: 0x%x, ptr: %d, len_tag_l: %d", length, decode_ptr, len_tag_len);
     //LOG(buf<<endl);

    decode_ptr=apimsg_body;
    memcpy(&type, decode_ptr, sizeof(char));
    decode_ptr++;

    if(type==PCE_USERCONSTRAINT)
    {
    	user_cons = new Apimsg_user_constraint();
    	length=pri_type_decoder.getLen(decode_ptr, len_tag_len);
        //decode_ptr=decode_ptr+len_tag_len;
    	this->decode_usercons(decode_ptr,length,user_cons);
    }
    length_decoded=length_decoded+1+len_tag_len+length;
    if(length_decoded!=msg_length)
    {
    	decode_ptr=decode_ptr+length; // point to the next field RESV_CONSTRAINT
    	memcpy(&type, decode_ptr++, sizeof(char));
    	if(type==PCE_RESVCONSTRAINT)
    	{
    		length=pri_type_decoder.getLen(decode_ptr, len_tag_len);
    		this->decode_resvcons(decode_ptr,length,user_cons);
    	}
    }


    return user_cons;


}


void Apireqmsg_decoder::decode_usercons(char* decode_ptr, int total_len, Apimsg_user_constraint* user_cons)
{
	int length=0;
	int starttime=0;
	int endtime=0;
	int bandwidth=0;
    //float bandwidth=0;
    //double bandwidth=0;
	int src_ip_port=0;
	int dest_ip_port=0;
	string path_setup_model="";
	string path_type="";
	string gri="";
	string layer="";
	string src_end_point="";
	string dest_end_point="";
	string src_vlan_tag="";
	string dest_vlan_tag="";
	string protocol="";
	string dscp="";
	int burst_limit=0;
	string lsp_class="";

    char bytevalue=0;
	int intvalue=0;
	int length_offset=0;
	u_int8_t type=0;
	int len_tag_len=0;





	while(length_offset<total_len)
	{
		memcpy(&type, decode_ptr++, sizeof(char));

		switch(type)
		{
		case PCE_STARTTIME:
			length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
			starttime = pri_type_decoder.decodeInt(decode_ptr,length);

			break;
		case PCE_ENDTIME:
			length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
			endtime = pri_type_decoder.decodeInt(decode_ptr,length);

			break;
		case PCE_BANDWIDTH:
			length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
			bandwidth = pri_type_decoder.decodeInt(decode_ptr,length);

			break;
		case PCE_PATHSETUPMODEL:
			length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
			path_setup_model = pri_type_decoder.decodeStr(decode_ptr,length);

			break;
		case PCE_PATHTYPE:
			length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
			path_type = pri_type_decoder.decodeStr(decode_ptr,length);

			break;
		case PCE_GRI:
			length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
			gri = pri_type_decoder.decodeStr(decode_ptr,length);
			break;
		case PCE_LAYER:
			length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
			layer = pri_type_decoder.decodeStr(decode_ptr,length);
			break;
		case PCE_SOURCE:
			length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
			src_end_point = pri_type_decoder.decodeStr(decode_ptr,length);
			break;
		case PCE_DESTINATION:
			length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
			dest_end_point = pri_type_decoder.decodeStr(decode_ptr,length);
			break;
		case PCE_SRCVLAN:
			length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
			src_vlan_tag = pri_type_decoder.decodeStr(decode_ptr,length);
			break;
		case PCE_DESTVLAN:
			length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
			dest_vlan_tag = pri_type_decoder.decodeStr(decode_ptr,length);
			break;
		case PCE_SRCIPPORT:
			length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
			src_ip_port = pri_type_decoder.decodeInt(decode_ptr,length);
			break;
		case PCE_DESTIPPORT:
			length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
			dest_ip_port = pri_type_decoder.decodeInt(decode_ptr,length);
			break;
		case PCE_L3_PROTOCOL:
			length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
			protocol = pri_type_decoder.decodeStr(decode_ptr,length);
			break;
		case PCE_L3_DSCP:
			length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
			dscp = pri_type_decoder.decodeStr(decode_ptr,length);
			break;
		case PCE_MPLS_BURSTLIMIT:
			length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
			burst_limit = pri_type_decoder.decodeInt(decode_ptr,length);
			break;
		case PCE_MPLS_LSPCLASS:
			length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
			lsp_class = pri_type_decoder.decodeStr(decode_ptr,length);
			break;
		}//end of switch

	length_offset=length_offset+length+len_tag_len+1;//should consider long form of length field
    

	}

	user_cons->setGri(gri);
    user_cons->setStarttime(starttime);
    user_cons->setEndtime(endtime);
    user_cons->setBandwidth(bandwidth);
    user_cons->setPathsetupmodel(path_setup_model);
    user_cons->setPathtype(path_type);
    user_cons->setLayer(layer);
    user_cons->setSrcendpoint(src_end_point);
    user_cons->setDestendpoint(dest_end_point);

    if(layer == "2")
    {
    	user_cons->setSrcvlantag(src_vlan_tag);
    	user_cons->setDestvlantag(dest_vlan_tag);

    }

    if(layer == "3")
    {
    	user_cons->setSrcipport(src_ip_port);
    	user_cons->setDestipport(dest_ip_port);
    	user_cons->setProtocol(protocol);
    	user_cons->setDscp(dscp);
    	user_cons->setBurstlimit(burst_limit);
    	user_cons->setLspclass(lsp_class);
    }


    //for debug issue output result to file
	string debug_output_path = getenv("HOME");
	string debug_filename = "/testoutput.txt";
	string debug_output = debug_output_path+debug_filename;


    ofstream outfile(debug_output.c_str(), ios::out);
    if(! outfile)
    {
        cerr<<"open error!"<<endl;
        exit(1);
    }

    outfile<<"gri="<<gri<<endl;
    outfile<<"starttime="<<starttime<<endl;
    outfile<<"endtime="<<endtime<<endl;
    outfile<<"bandwidth="<<bandwidth<<endl;
    outfile<<"path_setup_model="<<path_setup_model<<endl;
    outfile<<"path_type="<<path_type<<endl;
    outfile<<"layer="<<layer<<endl;
    outfile<<"src_end_point="<<src_end_point<<endl;
    outfile<<"dest_end_point="<<dest_end_point<<endl;

    if(layer == "2")
    {
    	outfile<<"src_vlan_tag="<<src_vlan_tag<<endl;
    	outfile<<"dest_vlan_tag="<<dest_vlan_tag<<endl;
    }

    if(layer == "3")
    {
    	outfile<<"src_ip_port="<<src_ip_port<<endl;
    	outfile<<"dest_ip_port="<<dest_ip_port<<endl;
    	outfile<<"protocol="<<protocol<<endl;
    	outfile<<"dscp="<<dscp<<endl;
    	outfile<<"burst_limit="<<burst_limit<<endl;
    	outfile<<"lsp_class="<<lsp_class<<endl;
    }






    outfile.close();


}


void Apireqmsg_decoder::decode_resvcons(char* decode_ptr, int total_len, Apimsg_user_constraint* user_cons)
{
	int length=0;
	int starttime=0;
	int endtime=0;
	int bandwidth=0;

    char bytevalue=0;
	int intvalue=0;
	int length_offset=0;
	u_int8_t type=0;
	int len_tag_len=0;
	char buf[API_MAX_MSG_SIZE];
	int i=0;

	string path_id="";
	int path_length=0;
	Path_req* path=NULL;
	Hop_req* hops=NULL;
	string hop_id="";
	string link_id="";
	string switching_cap_type="";
	string encoding_type="";
	string vlanRange_availability="";








	while(length_offset<total_len)
	{
		memcpy(&type, decode_ptr++, sizeof(char));

		switch(type)
		{
		case PCE_STARTTIME:
			length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
			starttime = pri_type_decoder.decodeInt(decode_ptr,length);

			break;
		case PCE_ENDTIME:
			length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
			endtime = pri_type_decoder.decodeInt(decode_ptr,length);

			break;
		case PCE_BANDWIDTH:
			length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
			bandwidth = pri_type_decoder.decodeInt(decode_ptr,length);

			break;
		case PCE_PATH_ID:
			length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
			path_id = pri_type_decoder.decodeStr(decode_ptr,length);

			break;

		}//end of switch


		length_offset=length_offset+length+len_tag_len+1;//should consider long form of length field

		if(type==PCE_PATH_ID)
		{
			break;
		}


	}

	if(type==PCE_PATH_ID)
	{
		path = new Path_req();
	}

	if(path!=NULL)
	{
		path->setPathid(path_id);
		memcpy(&type, decode_ptr++, sizeof(char));

		if(type!=PCE_PATH_LENGTH)
		{
	        snprintf(buf, 128, "API request message reserved constraint no path length");
	        LOG(buf << endl);
	        throw APIException(buf);
		}

		length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
		path_length = pri_type_decoder.decodeInt(decode_ptr,length);
		length_offset=length_offset+length+len_tag_len+1;

		path->setPathlength(path_length);

		if(path_length!=0)
		{
			hops = new Hop_req[path_length];
			path->setHops(hops);

			for(i=0;i<path_length;i++)
			{
				memcpy(&type, decode_ptr++, sizeof(char));
				if(type!=PCE_HOP_ID)
				{
			        snprintf(buf, 128, "API request message reserved constraint no hop id");
			        LOG(buf << endl);
			        throw APIException(buf);
				}
				length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
				hop_id = pri_type_decoder.decodeStr(decode_ptr,length);
				length_offset=length_offset+length+len_tag_len+1;
				hops[i].setHopid(hop_id);

				memcpy(&type, decode_ptr++, sizeof(char));
				if(type!=PCE_LINK_ID)
				{
			        snprintf(buf, 128, "API request message reserved constraint no link id");
			        LOG(buf << endl);
			        throw APIException(buf);
				}
				length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
				link_id = pri_type_decoder.decodeStr(decode_ptr,length);
				length_offset=length_offset+length+len_tag_len+1;
				hops[i].setLinkid(link_id);

				memcpy(&type, decode_ptr++, sizeof(char));
				if(type!=PCE_SWITCHINGCAPTYPE)
				{
			        snprintf(buf, 128, "API request message reserved constraint no switching capability type");
			        LOG(buf << endl);
			        throw APIException(buf);
				}
				length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
				switching_cap_type = pri_type_decoder.decodeStr(decode_ptr,length);
				length_offset=length_offset+length+len_tag_len+1;
				hops[i].setSwitchingcaptype(switching_cap_type);

				memcpy(&type, decode_ptr++, sizeof(char));
				if(type!=PCE_SWITCHINGENCTYPE)
				{
			        snprintf(buf, 128, "API request message reserved constraint no switching encoding type");
			        LOG(buf << endl);
			        throw APIException(buf);
				}
				length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
				encoding_type = pri_type_decoder.decodeStr(decode_ptr,length);
				length_offset=length_offset+length+len_tag_len+1;
				hops[i].setEncodingtype(encoding_type);

				memcpy(&type, decode_ptr++, sizeof(char));
				if(type!=PCE_SWITCHINGVLANRANGEAVAI)
				{
			        snprintf(buf, 128, "API request message reserved constraint no switching vlan range availability");
			        LOG(buf << endl);
			        throw APIException(buf);
				}
				length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
				vlanRange_availability = pri_type_decoder.decodeStr(decode_ptr,length);
				length_offset=length_offset+length+len_tag_len+1;
				hops[i].setVlanrangeavailability(vlanRange_availability);

			}

		}

		user_cons->setPath(path);

	}





    //for debug issue output result to file
	string debug_output_path = getenv("HOME");
	string debug_filename = "/testoutput.txt";
	string debug_output = debug_output_path+debug_filename;


    ofstream outfile(debug_output.c_str(), ios::out | ios::app);
    if(! outfile)
    {
        cerr<<"open error!"<<endl;
        exit(1);
    }

    outfile<<"reserved constraint"<<endl;
    outfile<<"starttime="<<starttime<<endl;
    outfile<<"endtime="<<endtime<<endl;
    outfile<<"bandwidth="<<bandwidth<<endl;

    if(user_cons->getPath()!=NULL)
    {
    	path = user_cons->getPath();
    	outfile<<"path id="<<path->getPathid()<<endl;
    	outfile<<"path length="<<path->getPathlength()<<endl;

    	if(path->getPathlength()!=0)
    	{
    		hops = path->getHops();
    		for(i=0;i<path->getPathlength();i++)
    		{
    			outfile<<"hop id="<<hops->getHopid()<<endl;
    			outfile<<"link id="<<hops->getLinkid()<<endl;
    			outfile<<"switching capability type="<<hops->getSwitchingcaptype()<<endl;
    			outfile<<"encoding type="<<hops->getEncodingtype()<<endl;
    			outfile<<"vlan range availability="<<hops->getVlanrangeavailability()<<endl;
    		}
    	}

    }



    outfile.close();


}
		
