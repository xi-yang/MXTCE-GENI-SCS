#include "reply_encoder.hh"
#include <fstream>
#include <stdlib.h>
#include "log.hh"

int Apireplymsg_encoder::msg_seq_num=0;

int Apireplymsg_encoder::test_encode_msg(Message* msg, char*& body)
{
	int msg_sublen=0;
	int msg_sub_startlen=0;
	int msg_len=0;
	u_int8_t* msg_sub_ptr;
	u_int8_t* msg_sub_start_ptr;
	u_int8_t* msg_body_ptr;
	u_int8_t* msg_pre_part_ptr;
	u_int8_t* msg_new_part_ptr;
	u_int8_t pceType=PCE_REPLY;
	u_int16_t type;
	u_int16_t length;
	u_int8_t* value;

	ComputeResult* compute_result;

	string gri="";
	string err_msg="";
	TPath* path_info=NULL;
	list<TLink*> path;

	TLink* link=NULL;
	ISCD* sw_cap_descriptors=NULL;
	Link* remoteLink=NULL;
    u_char	switchingType;
    u_char	encodingType;
    ConstraintTagSet* availableVlanTags;
    ConstraintTagSet* suggestedVlanTags;
    ConstraintTagSet* assignedVlanTags;
    bool vlanTranslation;
    int mtu;
    int capacity;
    string rangStr="";
    string remoteLinkName="";
    int metric;
    long maxBandwidth;
    long maxReservableBandwidth;
    long minReservableBandwidth;
    long unreservedBandwidth[8];   // 8 priorities: use unreservedBandwidth[7] by default
    long bandwidthGranularity;
    bool optional_cons_flag=true;

    list<TPath*> alterPaths;
    BandwidthAvailabilityGraph* bag=NULL;

    //Encode_Pri_Type* pri_type_encoder = new Encode_Pri_Type();

	char print_buff[200];

    memcpy(&compute_result, msg->GetTLVList().front()->value, sizeof(void*));

    //cout<<"int="<<sizeof(int)<<" long="<<sizeof(long)<<endl;

	gri = compute_result->GetGri();
	this->gri_value = gri;
	cout<<"gri="<<gri<<endl;
	pri_type_encoder->encodeString(PCE_GRI, gri);
	err_msg = compute_result->GetErrMessage();
	if(err_msg=="")
	{
		cout<<"error msg is empty string"<<endl;
		cout<<"error msg="<<err_msg<<endl;

		msg_sub_ptr=pri_type_encoder->get_buff();
		msg_sublen=pri_type_encoder->get_length();
		msg_pre_part_ptr = new u_int8_t[msg_sublen];
		memcpy(msg_pre_part_ptr, msg_sub_ptr, msg_sublen);
		this->length+=msg_sublen;
		pri_type_encoder->reset_length(); //reset encoder offset

		path_info = compute_result->GetPathInfo();
		if(path_info == NULL)
		{
			cout<<"path info is null"<<endl;
		}
	    if (path_info != NULL)
	    {
	    	encode_path(path_info, pri_type_encoder, 0);
	    }

		msg_sub_ptr=pri_type_encoder->get_buff();
		msg_sublen=pri_type_encoder->get_length();
		msg_sub_startlen=0;
		msg_sub_start_ptr=encode_msg_sub_start(PCE_REGU_REPLY, msg_sublen, msg_sub_startlen); //encode the subfield-header
		msg_body_ptr=encode_merge_buff(msg_sub_start_ptr, msg_sub_startlen, msg_sub_ptr, msg_sublen);  //merge subfield-header and body
		msg_len=msg_sub_startlen + msg_sublen; //body length
		delete[] msg_sub_start_ptr;
		msg_new_part_ptr=encode_merge_buff(msg_pre_part_ptr, this->length, msg_body_ptr, msg_len);  //merge the previous part and the new part
		delete[] msg_body_ptr;
		delete[] msg_pre_part_ptr;
		msg_pre_part_ptr=msg_new_part_ptr;
		this->length+=msg_len;
		pri_type_encoder->reset_length(); //reset encoder offset


	    if(optional_cons_flag == true)
	    {
	    	cout<<"optional constraint below"<<endl;
	    	//pri_type_encoder->encodeString(PCE_OPTI_REPLY, "opti");
	    	if(path_info != NULL)
	    	{
	    		encode_path(path_info, pri_type_encoder, 1);
	    	}

	    	alterPaths = compute_result->GetAlterPaths();

	    	if(!alterPaths.empty())
	    	{
	    		int alter_path_num = alterPaths.size();
	    		int i=0;

	    		cout<<"alternate path below"<<endl;
	    		pri_type_encoder->encodeInteger(PCE_ALT_PATH_NUM,alter_path_num);
	    		cout<<"alternate path number="<<alter_path_num<<endl;

	    		for(list<TPath*>::iterator it_path=alterPaths.begin();it_path!=alterPaths.end();it_path++)
	    		{
	    			if((*it_path)!=NULL)
	    			{
	    				encode_path((*it_path), pri_type_encoder, 1);

	    			}
	    		}
	    	}
	    	else
	    	{
	    		pri_type_encoder->encodeInteger(PCE_ALT_PATH_NUM,0); //set to 0 for decode side convenience

	    	}

	    	msg_sub_ptr=pri_type_encoder->get_buff();
	    	msg_sublen=pri_type_encoder->get_length();
	    	msg_sub_startlen=0;
	    	msg_sub_start_ptr=encode_msg_sub_start(PCE_OPTI_REPLY, msg_sublen, msg_sub_startlen); //encode the subfield-header
	    	msg_body_ptr=encode_merge_buff(msg_sub_start_ptr, msg_sub_startlen, msg_sub_ptr, msg_sublen);  //merge subfield-header and body
	    	msg_len=msg_sub_startlen + msg_sublen; //body length
	    	delete[] msg_sub_start_ptr;
	    	msg_new_part_ptr=encode_merge_buff(msg_pre_part_ptr, this->length, msg_body_ptr, msg_len);  //merge the previous part and the new part
	    	delete[] msg_body_ptr;
	    	delete[] msg_pre_part_ptr;
	    	msg_pre_part_ptr=msg_new_part_ptr;
	    	this->length+=msg_len;
	    	pri_type_encoder->reset_length(); //reset encoder offset
	    }
	}
	else
	{
		cout<<"error msg="<<err_msg<<endl;
		pri_type_encoder->encodeString(PCE_COMPUTE_ERROR, err_msg);

		msg_sub_ptr=pri_type_encoder->get_buff();
		msg_sublen=pri_type_encoder->get_length();
		msg_pre_part_ptr = new u_int8_t[msg_sublen];
		memcpy(msg_pre_part_ptr, msg_sub_ptr, msg_sublen);
		this->length+=msg_sublen;
		pri_type_encoder->reset_length(); //reset encoder offset
	}

	msg_sub_startlen=0;
	msg_sub_start_ptr=encode_msg_sub_start(PCE_REPLY, this->length, msg_sub_startlen); //encode the subfield-header
	msg_new_part_ptr=encode_merge_buff(msg_sub_start_ptr, msg_sub_startlen, msg_pre_part_ptr, this->length);  //merge subfield-header and body
	this->length=this->length+msg_sub_startlen;
	delete[] msg_sub_start_ptr;
	delete[] msg_pre_part_ptr;
	msg_pre_part_ptr=msg_new_part_ptr;

	//memory revoke for the buff in pri encoder
	//now move the function to deconstructor of pri encoder itself
	//msg_sub_ptr=pri_type_encoder->get_buff();
	//delete[] msg_sub_ptr;

	body=(char*)msg_pre_part_ptr;


	/*
    ofstream outfile("/home/wind/encodebin.txt", ios::binary);
    if(! outfile)
    {
        cerr<<"open error!"<<endl;
        exit(1);
    }

    outfile.write((char*)body, msg_len);
    outfile.close();
*/

	return this->length;

}

u_int8_t* Apireplymsg_encoder::encode_msg_sub_start(u_int8_t pceType, int msg_sublen, int& sub_start_len)
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

u_int8_t* Apireplymsg_encoder::encode_merge_buff(u_int8_t* buff1, int len1, u_int8_t* buff2, int len2)
{
	u_int8_t* buff_tmp = new u_int8_t[len1+len2];
	memcpy(buff_tmp, buff1, len1);
	memcpy(buff_tmp+len1, buff2, len2);
	return buff_tmp;
}

void Apireplymsg_encoder::encode_msg_header(api_msg_header& apimsg_header, int msg_body_len)
{
	u_int16_t len=0;
	len = (u_int16_t)msg_body_len;
	apimsg_header.type = htons(API_MSG_REPLY);
	apimsg_header.length = htons(len);
	apimsg_header.ucid = htonl(0x0002);
	apimsg_header.seqnum = htonl(msg_seq_num);
	apimsg_header.chksum = MSG_CHKSUM(apimsg_header);
	apimsg_header.options = htonl(0x0000);
	apimsg_header.tag = htonl(0x0000);
	msg_seq_num++;


}

void Apireplymsg_encoder::encode_path(TPath* path_info, Encode_Pri_Type* pri_type_encoder_ptr, int opti_flag)
{

	int msg_sublen=0;
	int msg_sub_startlen=0;
	int msg_len=0;
	u_int8_t* msg_sub_ptr;
	u_int8_t* msg_sub_start_ptr;
	u_int8_t* msg_body_ptr;
	u_int8_t pceType=PCE_REPLY;
	u_int16_t type;
	u_int16_t length;
	u_int8_t* value;

	string gri="";
	string err_msg="";
	list<TLink*> path;

	TLink* link=NULL;
	ISCD* sw_cap_descriptors=NULL;
	Link* remoteLink=NULL;
	u_char	switchingType;
	u_char	encodingType;
	ConstraintTagSet* availableVlanTags;
	ConstraintTagSet* suggestedVlanTags;
	ConstraintTagSet* assignedVlanTags;
	bool vlanTranslation;
	int mtu;
	int capacity;
	string rangStr="";
	string remoteLinkName="";
	int metric;
	long maxBandwidth;
	long maxReservableBandwidth;
	long minReservableBandwidth;
	long unreservedBandwidth[8];   // 8 priorities: use unreservedBandwidth[7] by default
	long bandwidthGranularity;
	bool optional_cons_flag=true;

	list<TPath*> alterPaths;

	BandwidthAvailabilityGraph* bag=NULL;

	//map<time_t, long> TBSF;

	map<time_t, long>::iterator it;

	char print_buff[200];

	path = path_info->GetPath();

	pri_type_encoder_ptr->encodeString(PCE_PATH_ID, "path-1");

	cout<<"path length="<<path.size()<<endl;

	pri_type_encoder_ptr->encodeInteger(PCE_PATH_LENGTH, path.size());

	for(list<TLink*>::iterator it=path.begin();it!=path.end();it++)
	{
		cout<<"id="<<(*it)->GetId()<<endl;
		cout<<"name="<<(*it)->GetName()<<endl;
		pri_type_encoder_ptr->encodeString(PCE_LINK_ID, (*it)->GetName());//encode link id (name)
		//cout<<"remote link="<<((*it)->GetRemoteLink())->GetName()<<endl;
		remoteLink = (*it)->GetRemoteLink();
		if(remoteLink != NULL)
		{
			remoteLinkName = remoteLink->GetName();
			pri_type_encoder_ptr->encodeString(PCE_REMOTE_LINK, remoteLinkName);
		}
		//remoteLinkName = ((*it)->GetRemoteLink())->GetName();
		cout<<"remote link="<<remoteLinkName<<endl;

		//pri_type_encoder_ptr->encodeString(PCE_REMOTE_LINK, remoteLinkName);

		maxReservableBandwidth = (*it)->GetMaxReservableBandwidth();
		pri_type_encoder_ptr->encodeInteger(PCE_MAXRESVCAPACITY, (int)maxReservableBandwidth);

		minReservableBandwidth = (*it)->GetMinReservableBandwidth();
		pri_type_encoder_ptr->encodeInteger(PCE_MINRESVCAPACITY, (int)minReservableBandwidth);

		bandwidthGranularity = (*it)->GetBandwidthGranularity();
		pri_type_encoder_ptr->encodeInteger(PCE_GRANULARITY, bandwidthGranularity);

		metric = (*it)->GetMetric();
		pri_type_encoder_ptr->encodeInteger(PCE_TE_METRIC, metric);

		sw_cap_descriptors=(*it)->GetTheISCD();

		cout<<"switchingtype="<<(int)sw_cap_descriptors->switchingType<<endl;
		cout<<"encodingtype="<<(int)sw_cap_descriptors->encodingType<<endl;
		cout<<"capacity="<<sw_cap_descriptors->capacity<<endl;

		capacity = sw_cap_descriptors->capacity;

		pri_type_encoder_ptr->encodeInteger(PCE_CAPACITY, capacity);

		switchingType = sw_cap_descriptors->switchingType;
		encodingType = sw_cap_descriptors->encodingType;

		switch (switchingType)
		{
		case LINK_IFSWCAP_L2SC:
			cout<<"mtu="<<((ISCD_L2SC*)sw_cap_descriptors)->mtu<<endl;
			cout<<"vlantranslation="<<((ISCD_L2SC*)sw_cap_descriptors)->vlanTranslation<<endl;

			mtu = ((ISCD_L2SC*)sw_cap_descriptors)->mtu;

			pri_type_encoder_ptr->encodeInteger(PCE_MTU, mtu);

			vlanTranslation = ((ISCD_L2SC*)sw_cap_descriptors)->vlanTranslation;

			pri_type_encoder_ptr->encodeBoolean(PCE_VLANTRANSLATION, vlanTranslation);

			pri_type_encoder_ptr->encodeString(PCE_SWITCHINGCAPTYPE, "l2sc");

			if(!(((ISCD_L2SC*)sw_cap_descriptors)->assignedVlanTags).IsEmpty())
			{
				cout<<"assignedvlantags is not empty"<<endl;
			}
			if(!(((ISCD_L2SC*)sw_cap_descriptors)->suggestedVlanTags).IsEmpty())
			{
				cout<<"suggestedvlantags is not empty"<<endl;
			}
			if(!(((ISCD_L2SC*)sw_cap_descriptors)->availableVlanTags).IsEmpty())
			{
				cout<<"availablevlantags is not empty"<<endl;
			}

			availableVlanTags = &((ISCD_L2SC*)sw_cap_descriptors)->availableVlanTags;
			if(!availableVlanTags->IsEmpty())
			{
				rangStr=availableVlanTags->GetRangeString();
				pri_type_encoder_ptr->encodeString(PCE_SWITCHINGVLANRANGEAVAI, rangStr);
				cout<<"availableVlanTags="<<rangStr<<endl;
			}
			suggestedVlanTags = &((ISCD_L2SC*)sw_cap_descriptors)->suggestedVlanTags;
			if(!suggestedVlanTags->IsEmpty())
			{
				rangStr=suggestedVlanTags->GetRangeString();
				pri_type_encoder_ptr->encodeString(PCE_SWITCHINGVLANRANGESUGG, rangStr);
				cout<<"suggestedVlanTags="<<rangStr<<endl;
			}
			assignedVlanTags = &((ISCD_L2SC*)sw_cap_descriptors)->assignedVlanTags;
			if(!assignedVlanTags->IsEmpty())
			{
				rangStr=assignedVlanTags->GetRangeString();
				pri_type_encoder_ptr->encodeString(PCE_SWITCHINGVLANRANGEASSI, rangStr);
				cout<<"assignedVlanTags="<<rangStr<<endl;
			}

			//iscd = new ISCD_L2SC(capacity, mtu);
			//((ISCD_L2SC*)iscd)->availableVlanTags.LoadRangeString(vlanRange);
			//((ISCD_L2SC*)iscd)->vlanTranslation = vlanTranslation;
			break;
		case LINK_IFSWCAP_PSC1:
			//iscd = new ISCD_PSC(1, capacity, mtu);
			cout<<"LINK_IFSWCAP_PSC1"<<endl;
			break;
		case LINK_IFSWCAP_TDM:
			//iscd = new ISCD_TDM(capacity, minBandwidth);
			//((ISCD_TDM*)iscd)->availableTimeSlots.LoadRangeString(timeslotRange);
			cout<<"LINK_IFSWCAP_TDM"<<endl;
			break;
		case LINK_IFSWCAP_LSC:
			//iscd = new ISCD_LSC(capacity);
			//((ISCD_LSC*)iscd)->availableWavelengths.LoadRangeString(wavelengthRange);
			//((ISCD_LSC*)iscd)->wavelengthTranslation = wavelengthTranslation;
			cout<<"LINK_IFSWCAP_LSC"<<endl;
			break;
		default:
			// type not supported
			cout<<"other"<<endl;
			//return NULL;
		}

		switch (encodingType)
		{
		case LINK_IFSWCAP_ENC_PKT:
			break;
		case LINK_IFSWCAP_ENC_ETH:
			pri_type_encoder_ptr->encodeString(PCE_SWITCHINGENCTYPE, "ethernet");
			break;
		case LINK_IFSWCAP_ENC_PDH:
			break;
		case LINK_IFSWCAP_ENC_RESV1:
			break;
		case LINK_IFSWCAP_ENC_SONETSDH:
			break;
		case LINK_IFSWCAP_ENC_RESV2:
			break;
		case LINK_IFSWCAP_ENC_DIGIWRAP:
			break;
		case LINK_IFSWCAP_ENC_LAMBDA:
			break;
		case LINK_IFSWCAP_ENC_FIBER:
			break;
		case LINK_IFSWCAP_ENC_RESV3:
			break;
		case LINK_IFSWCAP_ENC_FIBRCHNL:
			break;
		case LINK_IFSWCAP_ENC_G709ODUK:
			break;
		case LINK_IFSWCAP_ENC_G709OCH:
			break;
		default:
			break;
		}
	}



	if(opti_flag==1)
	{
		time_t new_time = 0;
		//time_t last_time = 0;
		long bandwidth;
		int bag_size;
		int counter=0;
		bag=path_info->GetBAG();
		map<time_t, long> TBSF=bag->GetTBSF();

		bag_size=TBSF.size();
		cout<<"size of bag="<<TBSF.size()<<endl;

		for(it=TBSF.begin();it!=TBSF.end();it++)
		{

			new_time = (*it).first;
			bandwidth = (*it).second;

			if(it!=TBSF.begin())
			{
				pri_type_encoder_ptr->encodeInteger(PCE_OPT_BAG_ENDTIME, new_time);
				cout<<"bag endtime="<<new_time<<endl;
			}

			if(counter!=(bag_size-1))
			{
				pri_type_encoder_ptr->encodeInteger(PCE_OPT_BAG_BANDWIDTH, bandwidth);
				pri_type_encoder_ptr->encodeInteger(PCE_OPT_BAG_STARTTIME, new_time);

				cout<<"bag bandwith="<<bandwidth<<endl;
				cout<<"bag starttime="<<new_time<<endl;
			}

			//last_time = new_time;
			counter++;

		}

		//encode the last endtime for bag segment
		//pri_type_encoder_ptr->encodeInteger(PCE_OPT_BAG_ENDTIME, last_time);
		//cout<<"bag endtime="<<last_time<<endl;

	}

	pri_type_encoder_ptr->encodeString(PCE_PATH_END_TAG, "pathend");

}
