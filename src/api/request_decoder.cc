
#include "request_decoder.hh"
#include <fstream>
#include <stdlib.h>
#include "log.hh"
#include "api.hh"


Apimsg_user_constraint* Apireqmsg_decoder::test_decode_msg(char* apimsg_body, int msg_length)
{
    char* decode_ptr;
    u_int8_t type;
    int len_tag_len = 0;
    Apimsg_user_constraint* user_cons=NULL;
    int length=0;
    int length_decoded=0;

    char buf[500];
    // snprintf(buf, 128, "length: 0x%x, ptr: %d, len_tag_l: %d", length, decode_ptr, len_tag_len);
     //LOG(buf<<endl);

    decode_ptr=apimsg_body;

    //printf("\n test= %d\n",decode_ptr);
    memcpy(&type, decode_ptr, sizeof(char));
    decode_ptr++;
    length_decoded++;

    if(type==PCE_USERCONSTRAINT)
    {
    	user_cons = new Apimsg_user_constraint();
    	length=pri_type_decoder.getLen(decode_ptr, len_tag_len);
        //decode_ptr=decode_ptr+len_tag_len;
    	this->decode_usercons(decode_ptr,length,user_cons);
    	cout<<"user constraint"<<endl;
    	length_decoded = length_decoded + len_tag_len + length;

    	//cout<<"first msg_leng="<<msg_length<<" decode_len"<<length_decoded<<endl;
    	//cout<<"len_tag_len="<<len_tag_len<<" leng="<<length;
    	//printf("\n test= %d\n",decode_ptr);

        while(length_decoded!=msg_length)
        {
        	memcpy(&type, decode_ptr, sizeof(char));
            decode_ptr++;
            length_decoded++;
        	if(type==PCE_RESERVEDCONSTRAINT)
    	    {
    	    	length=pri_type_decoder.getLen(decode_ptr, len_tag_len);
    	        //decode_ptr=decode_ptr+len_tag_len;
    	    	this->decode_resvcons(decode_ptr,length,user_cons);
    	    	cout<<"resv constraint"<<endl;
    	    	length_decoded = length_decoded + len_tag_len + length;
    	    	//cout<<"second len_tag_len="<<len_tag_len<<" length="<<length<<endl;
    	    }
        	else if(type==PCE_OPTICONSTRAINT_COSCHEDULEREQ)
        	{
        		length=pri_type_decoder.getLen(decode_ptr, len_tag_len);
        		this->decode_opitcons_coschedreq(decode_ptr,length,user_cons);
        		cout<<"opti constraint"<<endl;
        		length_decoded = length_decoded + len_tag_len + length;
        	}
        	//cout<<"msg_leng="<<msg_length<<" decode_len="<<length_decoded<<endl;
        	//printf("\n test= %d\n",decode_ptr);
        	//break;

        }




    }

    return user_cons;

}


void Apireqmsg_decoder::decode_usercons(char* & decode_ptr, int total_len, Apimsg_user_constraint* user_cons)
{
	int length=0;
	int starttime=0;
	int endtime=0;
	u_int64_t bandwidth=0;
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
			bandwidth = pri_type_decoder.decodeLong(decode_ptr,length);

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

	length_offset=length_offset+length+len_tag_len+1;//should consider u_int64_t form of length field
    

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

    cout<<"decode user cons"<<endl;


}


void Apireqmsg_decoder::decode_resvcons(char* & decode_ptr, int total_len, Apimsg_user_constraint* user_cons)
{
	int length=0;
	int starttime=0;
	int endtime=0;
	u_int64_t bandwidth=0;

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
	string suggested_vlan_range="";
	bool vlanTranslation=false;

	bool hop_flag=false;




	/*
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
			bandwidth = pri_type_decoder.decodeLong(decode_ptr,length);

			break;
		case PCE_PATH_ID:
			length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
			path_id = pri_type_decoder.decodeStr(decode_ptr,length);

			break;

		}//end of switch



		length_offset=length_offset+length+len_tag_len+1;//should consider u_int64_t form of length field

		//cout<<"length_offset="<<length_offset<<endl;
		//printf("\n test_2= %d\n",decode_ptr);

		//reset length and len_tag_len
		length = 0;
		len_tag_len = 0;

		if(type==PCE_PATH_ID)
		{
			break;
		}

	}
	*/

	memcpy(&type, decode_ptr++, sizeof(char));

	if(type==PCE_PATH_ID)
	{
		path = new Path_req();

		length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
		path_id = pri_type_decoder.decodeStr(decode_ptr,length);
		length_offset=length_offset+length+len_tag_len+1;

	}
	else
	{
        snprintf(buf, 128, "API request message reserved constraint the first field is not path id");
        LOG(buf << endl);
        throw APIException(buf);

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
				hop_flag=true;
				while(hop_flag)
				{
					memcpy(&type, decode_ptr++, sizeof(char));
					switch(type)
					{
					case PCE_HOP_ID:
						length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
						hop_id = pri_type_decoder.decodeStr(decode_ptr,length);
						length_offset=length_offset+length+len_tag_len+1;
						hops[i].setHopid(hop_id);

						break;
					case PCE_LINK_ID:
						length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
						link_id = pri_type_decoder.decodeStr(decode_ptr,length);
						length_offset=length_offset+length+len_tag_len+1;
						hops[i].setLinkid(link_id);

						break;
					case PCE_SWITCHINGCAPTYPE:
						length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
						switching_cap_type = pri_type_decoder.decodeStr(decode_ptr,length);
						length_offset=length_offset+length+len_tag_len+1;
						hops[i].setSwitchingcaptype(switching_cap_type);

						break;
					case PCE_SWITCHINGENCTYPE:
						length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
						encoding_type = pri_type_decoder.decodeStr(decode_ptr,length);
						length_offset=length_offset+length+len_tag_len+1;
						hops[i].setEncodingtype(encoding_type);

						break;
					case PCE_SWITCHINGVLANRANGEAVAI:
						length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
						vlanRange_availability = pri_type_decoder.decodeStr(decode_ptr,length);
						length_offset=length_offset+length+len_tag_len+1;
						hops[i].setVlanrangeavailability(vlanRange_availability);

						break;
					case PCE_SWITCHINGVLANRANGESUGG:
						length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
						suggested_vlan_range = pri_type_decoder.decodeStr(decode_ptr,length);
						length_offset=length_offset+length+len_tag_len+1;
						hops[i].setSuggestedvlanrange(suggested_vlan_range);

						break;
					case PCE_VLANTRANSLATION:
						length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
						vlanTranslation = pri_type_decoder.decodeBoolean(decode_ptr,length);
						length_offset=length_offset+length+len_tag_len+1;
						hops[i].setVlanTranslation(vlanTranslation);

						break;
					case PCE_HOP_END_TAG:
						length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
						pri_type_decoder.decodeInt(decode_ptr,length);
						length_offset=length_offset+length+len_tag_len+1;
						hop_flag=false;

						break;
					}

					/*
					if(type!=PCE_HOP_ID)
					{
						printf("type=%d\n",type);
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
					*/



				}


			}

		}

		user_cons->setPath(path);

	}

	//cout<<"length_offset="<<length_offset<<endl;
	//printf("\n test_2= %d\n",decode_ptr);



	cout<<"decode resv cons"<<endl;

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

    /*
    outfile<<"reserved constraint"<<endl;
    outfile<<"starttime="<<starttime<<endl;
    outfile<<"endtime="<<endtime<<endl;
    outfile<<"bandwidth="<<bandwidth<<endl;
    */

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
    			outfile<<"suggested vlan range="<<hops->getSuggestedvlanrange()<<endl;
    			outfile<<"vlan translation="<<hops->getVlanTranslation()<<endl;
    			hops = hops + 1;
    		}
    		hops = NULL;
    	}

    }



    outfile.close();


}


void Apireqmsg_decoder::decode_opitcons_coschedreq(char* & decode_ptr, int total_len, Apimsg_user_constraint* user_cons)
{
	int length=0;

    //char bytevalue=0;
	//int intvalue=0;
	int length_offset=0;
	u_int8_t type=0;
	int len_tag_len=0;
	//char buf[API_MAX_MSG_SIZE];
	//int i=0;


	string co_schedule_request_id="";
	u_int32_t start_time=0;
	u_int32_t end_time=0;
	u_int64_t min_bandwidth=0L;
	u_int32_t max_num_of_alt_paths=0;
	bool bandwidth_avai_graph=false;
	bool contiguous_vlan=false;
	int max_duration=0;
	u_int64_t max_bandwidth=0L;
	int data_size_bytes=0;

	Apimsg_stornet_constraint* co_schedule_request = new Apimsg_stornet_constraint();


	while(length_offset<total_len)
	{
		memcpy(&type, decode_ptr++, sizeof(char));

		switch(type)
		{
		case PCE_OPT_COSCHEDREQID:
			length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
			co_schedule_request_id = pri_type_decoder.decodeStr(decode_ptr,length);

			break;
		case PCE_OPT_COSCHREQ_STARTTIME:
			length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
			start_time = pri_type_decoder.decodeInt(decode_ptr,length);

			break;
		case PCE_OPT_COSCHREQ_ENDTIME:
			length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
			end_time = pri_type_decoder.decodeInt(decode_ptr,length);

			break;
		case PCE_OPT_COSCHREQ_MINBANDWIDTH:
			length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
			min_bandwidth = pri_type_decoder.decodeLong(decode_ptr,length);

			break;
		case PCE_OPT_COSCHREQ_MAXNUMOFALTPATHS:
			length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
			max_num_of_alt_paths = pri_type_decoder.decodeInt(decode_ptr,length);

			break;
		case PCE_OPT_COSCHREQ_BANDWIDTHAVAIGRAPH:
			length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
			bandwidth_avai_graph = pri_type_decoder.decodeBoolean(decode_ptr, length);

			break;
		case PCE_OPT_COSCHREQ_CONTIGUOUSVLAN:
			length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
			contiguous_vlan = pri_type_decoder.decodeBoolean(decode_ptr, length);

			break;
		case PCE_OPT_COSCHREQ_MAXDURATION:
			length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
			max_duration = pri_type_decoder.decodeInt(decode_ptr, length);

			break;
		case PCE_OPT_COSCHREQ_MAXBANDWIDTH:
			length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
			max_bandwidth = pri_type_decoder.decodeLong(decode_ptr, length);

			break;
		case PCE_OPT_COSCHREQ_DATASIZEBYTES:
			length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
			data_size_bytes = pri_type_decoder.decodeInt(decode_ptr, length);

			break;

		}//end of switch


		length_offset=length_offset+length+len_tag_len+1;//should consider u_int64_t form of length field


	}

	co_schedule_request->setCoschedulerequestid(co_schedule_request_id);
	co_schedule_request->setStarttime(start_time);
	co_schedule_request->setEndtime(end_time);
	co_schedule_request->setMinbandwidth(min_bandwidth);
	co_schedule_request->setMaxnumofaltpaths(max_num_of_alt_paths);
	co_schedule_request->setBandwidthavaigraph(bandwidth_avai_graph);
	co_schedule_request->setContiguousvlan(contiguous_vlan);
	co_schedule_request->setMaxduration(max_duration);
	co_schedule_request->setMaxbandwidth(max_bandwidth);
	co_schedule_request->setDatasizebytes(data_size_bytes);

	user_cons->setCoschedreq(co_schedule_request);


	cout<<"decode opti cons"<<endl;

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

    outfile<<"optional constraint"<<endl;
    outfile<<"start time="<<start_time<<endl;
    outfile<<"end time="<<end_time<<endl;
    outfile<<"min bandwidth="<<min_bandwidth<<endl;
    outfile<<"max num of alt paths="<<max_num_of_alt_paths<<endl;
    outfile<<"bandwidth availability graph="<<bandwidth_avai_graph<<endl;
    outfile<<"contiguous vlan="<<contiguous_vlan<<endl;
    outfile<<"max duration="<<max_duration<<endl;
    outfile<<"max bandwidth="<<max_bandwidth<<endl;
    outfile<<"data size bytes="<<data_size_bytes<<endl;

    outfile.close();


}
