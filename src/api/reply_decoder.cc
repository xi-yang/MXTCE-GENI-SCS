/*
 * reply_decoder.cc
 *
 *  Created on: May 18, 2011
 *      Author: wind
 */

#include "reply_decoder.hh"

Compute_result* Apireplymsg_decoder::decode_reply_msg(char* apimsg_body, int msg_len)
{
    char* decode_ptr;
    u_int8_t type;
    int len_tag_len = 0;
    Compute_result* compute_res= new Compute_result();
    int length=0;
    int length_decoded=0;

    char buf[500];
    // snprintf(buf, 128, "length: 0x%x, ptr: %d, len_tag_l: %d", length, decode_ptr, len_tag_len);
     //LOG(buf<<endl);

    decode_ptr=apimsg_body;
    memcpy(&type, decode_ptr, sizeof(char));
    decode_ptr++;
    length_decoded++;

    if(type==PCE_REPLY)
    {
    	length=pri_type_decoder.getLen(decode_ptr, len_tag_len);
    	this->decode_compute_result(decode_ptr,length,compute_res);
    	cout<<"user constraint"<<endl;
    	length_decoded = length_decoded + len_tag_len + length;

    }

    return compute_res;

}

void Apireplymsg_decoder::decode_compute_result(char* & decode_ptr, int total_len, Compute_result* compute_res)
{

;
	string gri="";
	string path_name="";
	int path_len=0;
	Link_info* link=NULL;
	string link_name="";
	string switching_type="";
	string encoding_type="";
	string available_vlan_tags="";
	string suggested_vlan_tags="";
	string assigned_vlan_tags="";
	bool vlan_translation=false;
	int capacity=0;
	int mtu=0;

	int length=0;
    char bytevalue=0;
	int intvalue=0;
	int length_offset=0;
	u_int8_t type=0;
	int len_tag_len=0;
	int decoded_len=0;


	memcpy(&type, decode_ptr++, sizeof(char));
	decoded_len++;
	if(type == PCE_GRI)
	{
		length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
		gri = pri_type_decoder.decodeStr(decode_ptr,length);
		decoded_len = decoded_len + len_tag_len + length;

		compute_res->setGri(gri);
	}

	memcpy(&type, decode_ptr++, sizeof(char));
	decoded_len++;
	if(type == PCE_PATH_ID)
	{
		length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
		path_name = pri_type_decoder.decodeStr(decode_ptr,length);
		decoded_len = decoded_len + len_tag_len + length;

		memcpy(&type, decode_ptr++, sizeof(char));
		decoded_len++;
		if(type == PCE_PATH_LENGTH)
		{
			length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
			path_len = pri_type_decoder.decodeInt(decode_ptr,length);
			decoded_len = decoded_len + len_tag_len + length;

		}

		while(decoded_len<=total_len)
		{
			link = new Link_info();
			memcpy(&type, decode_ptr++, sizeof(char));
			decoded_len++;
			if(type == PCE_LINK_ID)
			{
				length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
				link_name = pri_type_decoder.decodeStr(decode_ptr,length);
				decoded_len = decoded_len + len_tag_len + length;
				link->setLinkName(link_name);

			}
			else if(type == PCE_SWITCHINGCAPTYPE)
			{
				length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
				switching_type = pri_type_decoder.decodeStr(decode_ptr,length);
				decoded_len = decoded_len + len_tag_len + length;
				link->setSwitchingType(switching_type);

			}
			else if(type == PCE_SWITCHINGENCTYPE)
			{
				length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
				encoding_type = pri_type_decoder.decodeStr(decode_ptr,length);
				decoded_len = decoded_len + len_tag_len + length;
				link->setEncodingType(encoding_type);

			}
			else if(type == PCE_SWITCHINGVLANRANGEAVAI)
			{
				length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
				available_vlan_tags = pri_type_decoder.decodeStr(decode_ptr,length);
				decoded_len = decoded_len + len_tag_len + length;
				link->setAvailableVlanTags(available_vlan_tags);

			}
			else if(type == PCE_SWITCHINGVLANRANGESUGG)
			{
				length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
				suggested_vlan_tags = pri_type_decoder.decodeStr(decode_ptr,length);
				decoded_len = decoded_len + len_tag_len + length;
				link->setSuggestedVlanTags(suggested_vlan_tags);

			}
			else if(type == PCE_SWITCHINGVLANRANGEASSI)
			{
				length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
				assigned_vlan_tags = pri_type_decoder.decodeStr(decode_ptr,length);
				decoded_len = decoded_len + len_tag_len + length;
				link->setAssignedVlanTags(assigned_vlan_tags);

			}
			else if(type == PCE_VLANTRANSLATION)
			{
				length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
				vlan_translation = pri_type_decoder.decodeBoolean(decode_ptr,length);
				decoded_len = decoded_len + len_tag_len + length;
				link->setVlanTranslation(vlan_translation);

			}
			else if(type == PCE_CAPACITY)
			{
				length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
				capacity = pri_type_decoder.decodeInt(decode_ptr,length);
				decoded_len = decoded_len + len_tag_len + length;
				link->setCapacity(capacity);

			}
			else if(type == PCE_MTU)
			{
				length = pri_type_decoder.getLen(decode_ptr, len_tag_len);
				mtu = pri_type_decoder.decodeInt(decode_ptr,length);
				decoded_len = decoded_len + len_tag_len + length;
				link->setMtu(mtu);

			}
			else
			{

			}
			compute_res->GetPath().push_back(link);
		}

	}

    //for debug issue output result to file
	string debug_output_path = getenv("HOME");
	string debug_filename = "/testoutputre.txt";
	string debug_output = debug_output_path+debug_filename;


    ofstream outfile(debug_output.c_str(), ios::out);
    if(! outfile)
    {
        cerr<<"open error!"<<endl;
        exit(1);
    }

    outfile<<"gri="<<gri<<endl;



    outfile.close();

    cout<<"decode user cons"<<endl;


}
