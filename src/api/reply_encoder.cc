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
	/*
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
    u_int64_t maxBandwidth;
    u_int64_t maxReservableBandwidth;
    u_int64_t minReservableBandwidth;
    u_int64_t unreservedBandwidth[8];   // 8 priorities: use unreservedBandwidth[7] by default
    u_int64_t bandwidthGranularity;
    */
    bool optional_cons_flag=true;

    list<TPath*> alterPaths;
    //BandwidthAvailabilityGraph* bag=NULL;

    //Encode_Pri_Type* pri_type_encoder = new Encode_Pri_Type();

	char print_buff[200];

    memcpy(&compute_result, msg->GetTLVList().front()->value, sizeof(void*));

    //cout<<"int="<<sizeof(int)<<" u_int64_t="<<sizeof(u_int64_t)<<endl;

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
		msg_pre_part_ptr = new u_int8_t[msg_sublen];  //create a new memory space to store the first part (gri)
		memcpy(msg_pre_part_ptr, msg_sub_ptr, msg_sublen); //copy the first part to allocated space
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
		msg_sub_startlen=0;  //this variable will be used as a reference argument in encode_msg_sub_start, it stores the length of msg header
		msg_sub_start_ptr=encode_msg_sub_start(PCE_REGU_REPLY, msg_sublen, msg_sub_startlen); //encode the subfield-header
		msg_body_ptr=encode_merge_buff(msg_sub_start_ptr, msg_sub_startlen, msg_sub_ptr, msg_sublen);  //merge subfield-header and body
		msg_len=msg_sub_startlen + msg_sublen; //body length
		delete[] msg_sub_start_ptr;
		msg_new_part_ptr=encode_merge_buff(msg_pre_part_ptr, this->length, msg_body_ptr, msg_len);  //merge the previous part and the new part
		delete[] msg_body_ptr;
		delete[] msg_pre_part_ptr;
		msg_pre_part_ptr=msg_new_part_ptr;  //msg_pre_part_ptr points to the newly merged result
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
	    	msg_pre_part_ptr=msg_new_part_ptr;  //msg_pre_part_ptr points to the newly merged result
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
		msg_pre_part_ptr = new u_int8_t[msg_sublen];  //create a new memory space to store the first part (gri and error msg)
		memcpy(msg_pre_part_ptr, msg_sub_ptr, msg_sublen);  //copy the first part to allocated space
		this->length+=msg_sublen;
		pri_type_encoder->reset_length(); //reset encoder offset
	}

	msg_sub_startlen=0;
	msg_sub_start_ptr=encode_msg_sub_start(PCE_REPLY, this->length, msg_sub_startlen); //encode the subfield-header
	msg_new_part_ptr=encode_merge_buff(msg_sub_start_ptr, msg_sub_startlen, msg_pre_part_ptr, this->length);  //merge subfield-header and body
	this->length=this->length+msg_sub_startlen;
	delete[] msg_sub_start_ptr;
	delete[] msg_pre_part_ptr;
	msg_pre_part_ptr=msg_new_part_ptr;  //msg_pre_part_ptr points to the newly merged result

	//memory revoke for the buff in pri encoder
	//now move the function to deconstructor of pri encoder itself
	//msg_sub_ptr=pri_type_encoder->get_buff();
	//delete[] msg_sub_ptr;

	body=(char*)msg_pre_part_ptr;  //store the pointer of the result to reference pointer parameter body


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
	//TLink* link=NULL;
	//ISCD* sw_cap_descriptors=NULL;
	bool optional_cons_flag=true;
	//list<TPath*> alterPaths;
	char print_buff[200];

	path = path_info->GetPath();

	pri_type_encoder_ptr->encodeString(PCE_PATH_ID, "path-1");

	cout<<"path length="<<path.size()<<endl;

	pri_type_encoder_ptr->encodeInteger(PCE_PATH_LENGTH, path.size());

	for(list<TLink*>::iterator it=path.begin();it!=path.end();it++)
	{
		int metric;
		u_int64_t maxBandwidth;
		u_int64_t maxReservableBandwidth;
		u_int64_t minReservableBandwidth;
		u_int64_t unreservedBandwidth[8];   // 8 priorities: use unreservedBandwidth[7] by default
		u_int64_t bandwidthGranularity;
		Link* remoteLink=NULL;
		string remoteLinkName="";
		list<ISCD*> swCapDescriptors;
		list<IACD*> adjCapDescriptors;

		cout<<"id="<<(*it)->GetId()<<endl;
		cout<<"name="<<(*it)->GetName()<<endl;
		pri_type_encoder_ptr->encodeString(PCE_LINK_ID, (*it)->GetName());//encode link id (name)
		//cout<<"remote link="<<((*it)->GetRemoteLink())->GetName()<<endl;
		remoteLink = (*it)->GetRemoteLink();
		if(remoteLink != NULL)
		{
			remoteLinkName = remoteLink->GetFullUrn();
			pri_type_encoder_ptr->encodeString(PCE_REMOTE_LINK, remoteLinkName);
		}
		//remoteLinkName = ((*it)->GetRemoteLink())->GetName();
		cout<<"remote link="<<remoteLinkName<<endl;

		//pri_type_encoder_ptr->encodeString(PCE_REMOTE_LINK, remoteLinkName);

		maxReservableBandwidth = (*it)->GetMaxReservableBandwidth();
		pri_type_encoder_ptr->encodeLong(PCE_MAXRESVCAPACITY, maxReservableBandwidth);

		minReservableBandwidth = (*it)->GetMinReservableBandwidth();
		pri_type_encoder_ptr->encodeLong(PCE_MINRESVCAPACITY, minReservableBandwidth);

		bandwidthGranularity = (*it)->GetBandwidthGranularity();
		pri_type_encoder_ptr->encodeLong(PCE_GRANULARITY, bandwidthGranularity);

		metric = (*it)->GetMetric();
		pri_type_encoder_ptr->encodeInteger(PCE_TE_METRIC, metric);

		swCapDescriptors = (*it)->GetSwCapDescriptors();

		for(list<ISCD*>::iterator iscdVar=swCapDescriptors.begin();iscdVar!=swCapDescriptors.end();iscdVar++)
		{
			u_char	switchingType;
			u_char	encodingType;
			u_int64_t capacity;
			string rangStr="";
			//sw_cap_descriptors=(*it)->GetTheISCD();

			switchingType = (*iscdVar)->switchingType;
			cout<<"switchingtype="<<(int)switchingType<<endl;

			encodingType = (*iscdVar)->encodingType;
			cout<<"encodingtype="<<(int)encodingType<<endl;

			//always encode switch capability type first
			switch (switchingType)
			{
			case LINK_IFSWCAP_L2SC:
			{
				ConstraintTagSet* availableVlanTags;
				ConstraintTagSet* suggestedVlanTags;
				ConstraintTagSet* assignedVlanTags;
				bool vlanTranslation;
				int mtu;

				pri_type_encoder_ptr->encodeString(PCE_SWITCHINGCAPTYPE, "l2sc");

				mtu = ((ISCD_L2SC*)(*iscdVar))->mtu;
				pri_type_encoder_ptr->encodeInteger(PCE_MTU, mtu);
				cout<<"mtu="<<mtu<<endl;

				vlanTranslation = ((ISCD_L2SC*)(*iscdVar))->vlanTranslation;
				pri_type_encoder_ptr->encodeBoolean(PCE_VLANTRANSLATION, vlanTranslation);
				cout<<"vlantranslation="<<vlanTranslation<<endl;

				/*
			if(!(((ISCD_L2SC*)(*iscdVar))->assignedVlanTags).IsEmpty())
			{
				cout<<"assignedvlantags is not empty"<<endl;
			}
			if(!(((ISCD_L2SC*)(*iscdVar))->suggestedVlanTags).IsEmpty())
			{
				cout<<"suggestedvlantags is not empty"<<endl;
			}
			if(!(((ISCD_L2SC*)(*iscdVar))->availableVlanTags).IsEmpty())
			{
				cout<<"availablevlantags is not empty"<<endl;
			}
				 */

				availableVlanTags = &((ISCD_L2SC*)(*iscdVar))->availableVlanTags;
				if(!availableVlanTags->IsEmpty())
				{
					rangStr=availableVlanTags->GetRangeString();
					pri_type_encoder_ptr->encodeString(PCE_SWITCHINGVLANRANGEAVAI, rangStr);
					cout<<"availableVlanTags="<<rangStr<<endl;
				}
				suggestedVlanTags = &((ISCD_L2SC*)(*iscdVar))->suggestedVlanTags;
				if(!suggestedVlanTags->IsEmpty())
				{
					rangStr=suggestedVlanTags->GetRangeString();
					pri_type_encoder_ptr->encodeString(PCE_SWITCHINGVLANRANGESUGG, rangStr);
					cout<<"suggestedVlanTags="<<rangStr<<endl;
				}
				assignedVlanTags = &((ISCD_L2SC*)(*iscdVar))->assignedVlanTags;
				if(!assignedVlanTags->IsEmpty())
				{
					rangStr=assignedVlanTags->GetRangeString();
					pri_type_encoder_ptr->encodeString(PCE_SWITCHINGVLANRANGEASSI, rangStr);
					cout<<"assignedVlanTags="<<rangStr<<endl;
				}
				break;
			}
			case LINK_IFSWCAP_PSC1:
			{
				//iscd = new ISCD_PSC(1, capacity, mtu);
				cout<<"LINK_IFSWCAP_PSC1"<<endl;
				break;
			}
			case LINK_IFSWCAP_TDM:
			{
				TDMConcatenationType concatenationType;
				ConstraintTagSet* availableTimeSlots;
				ConstraintTagSet* assignedTimeSlots;
				ConstraintTagSet* suggestedTimeSlots;
				bool tsiEnabled;
				bool vcatEnabled;
				string concatenationTypeStr="";

				pri_type_encoder_ptr->encodeString(PCE_SWITCHINGCAPTYPE, "tdm");
				concatenationType = ((ISCD_TDM*)(*iscdVar))->concatenationType;

				switch (concatenationType)
				{
				case STS1:
					concatenationTypeStr = "sts1";
					break;
				case STS3C:
					concatenationTypeStr = "sts3c";
				default:
					break;
				}

				pri_type_encoder_ptr->encodeString(PCE_CONCATENATIONTYPE,concatenationTypeStr);
				cout<<"concatenationTypeStr="<<concatenationTypeStr<<endl;

				availableTimeSlots = &((ISCD_TDM*)(*iscdVar))->availableTimeSlots;
				if(!availableTimeSlots->IsEmpty())
				{
					rangStr=availableTimeSlots->GetRangeString();
					pri_type_encoder_ptr->encodeString(PCE_SWITCHINGTIMESLOTAVAI, rangStr);
					cout<<"availableTimeSlots="<<rangStr<<endl;
				}

				assignedTimeSlots = &((ISCD_TDM*)(*iscdVar))->assignedTimeSlots;
				if(!assignedTimeSlots->IsEmpty())
				{
					rangStr=assignedTimeSlots->GetRangeString();
					pri_type_encoder_ptr->encodeString(PCE_SWITCHINGTIMESLOTASSI, rangStr);
					cout<<"assignedTimeSlots="<<rangStr<<endl;
				}

				suggestedTimeSlots = &((ISCD_TDM*)(*iscdVar))->suggestedTimeSlots;
				if(!suggestedTimeSlots->IsEmpty())
				{
					rangStr=suggestedTimeSlots->GetRangeString();
					pri_type_encoder_ptr->encodeString(PCE_SWITCHINGTIMESLOTSUGG, rangStr);
					cout<<"suggestedTimeSlots="<<rangStr<<endl;
				}

				tsiEnabled = ((ISCD_TDM*)(*iscdVar))->tsiEnabled;
				pri_type_encoder_ptr->encodeBoolean(PCE_TSIENABLED,tsiEnabled);
				cout<<"tsiEnabled="<<tsiEnabled<<endl;

				vcatEnabled = ((ISCD_TDM*)(*iscdVar))->vcatEnabled;
				pri_type_encoder_ptr->encodeBoolean(PCE_VCATENABLED,vcatEnabled);
				cout<<"vcatEnabled="<<vcatEnabled<<endl;
				break;
			}
			case LINK_IFSWCAP_LSC:
			{
				WDMChannelRepresentationType channelRepresentation;
				ConstraintTagSet* availableWavelengths;
				ConstraintTagSet* assignedWavelengths;
				ConstraintTagSet* suggestedWavelengths;
				bool wavelengthConversion;
				string channelRepresentationStr="";

				pri_type_encoder_ptr->encodeString(PCE_SWITCHINGCAPTYPE, "lsc");
				channelRepresentation = ((ISCD_LSC*)(*iscdVar))->channelRepresentation;

				switch (channelRepresentation)
				{
				case FREQUENCY_GHZ:
					channelRepresentationStr = "frequency-ghz";
					break;
				case WAVELENGTH_NM:
					channelRepresentationStr = "wavelength-nm";
					break;
				case ITU_GRID_100GHZ:
					channelRepresentationStr = "itu-channel-grid-100ghz";
					break;
				case ITU_GRID_50GHZ:
					channelRepresentationStr = "itu-channel-grid-50ghz";
					break;
				default:
					break;
				}

				pri_type_encoder_ptr->encodeString(PCE_CHANNELREPRESENTATION,channelRepresentationStr);
				cout<<"channelRepresentationStr="<<channelRepresentationStr<<endl;

				availableWavelengths = &((ISCD_LSC*)(*iscdVar))->availableWavelengths;
				if(!availableWavelengths->IsEmpty())
				{
					rangStr=availableWavelengths->GetRangeString();
					pri_type_encoder_ptr->encodeString(PCE_SWITCHINGWAVELENAVAI, rangStr);
					cout<<"availableWavelengths="<<rangStr<<endl;
				}

				assignedWavelengths = &((ISCD_LSC*)(*iscdVar))->assignedWavelengths;
				if(!assignedWavelengths->IsEmpty())
				{
					rangStr=assignedWavelengths->GetRangeString();
					pri_type_encoder_ptr->encodeString(PCE_SWITCHINGWAVELENASSI, rangStr);
					cout<<"assignedWavelengths="<<rangStr<<endl;
				}

				suggestedWavelengths = &((ISCD_LSC*)(*iscdVar))->suggestedWavelengths;
				if(!suggestedWavelengths->IsEmpty())
				{
					rangStr=suggestedWavelengths->GetRangeString();
					pri_type_encoder_ptr->encodeString(PCE_SWITCHINGWAVELENSUGG, rangStr);
					cout<<"suggestedWavelengths="<<rangStr<<endl;
				}

				wavelengthConversion = ((ISCD_LSC*)(*iscdVar))->wavelengthConversion;
				pri_type_encoder_ptr->encodeBoolean(PCE_WAVELENGTHCONVERSION,wavelengthConversion);
				cout<<"wavelengthConversion="<<wavelengthConversion<<endl;
				break;
			}
			default:
				// type not supported
				cout<<"other"<<endl;
				//return NULL;
				break;
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

			capacity = (*iscdVar)->capacity;
			pri_type_encoder_ptr->encodeLong(PCE_CAPACITY, capacity);
			cout<<"capacity="<<capacity<<endl;

			if ((*iscdVar)->vendorSpecInfoParser != NULL)
			{
				string xmlVendorSpecInfo= (*iscdVar)->vendorSpecInfoParser->GetXmlByString();
				pri_type_encoder_ptr->encodeString(PCE_VENDORSPECIFICINFO, xmlVendorSpecInfo);
				cout<<"vendorSpecificInfo="<<xmlVendorSpecInfo<<endl;
			}

		}

		adjCapDescriptors = (*it)->GetAdjCapDescriptors();
		for(list<IACD*>::iterator iacdVar=adjCapDescriptors.begin();iacdVar!=adjCapDescriptors.end();iacdVar++)
		{
		    u_char	lowerLayerSwitchingType;
		    u_char	lowerLayerEncodingType;
		    u_char  upperLayerSwitchingType;
		    u_char	upperLayerEncodingType;
		    u_int64_t maxAdaptBandwidth;

		    pri_type_encoder_ptr->encodeString(PCE_IACD_START,"IACD_START");

		    lowerLayerSwitchingType = (*iacdVar)->lowerLayerSwitchingType;
		    pri_type_encoder_ptr->encodeString(PCE_LOWERLAYERSWITCHINGTYPE, this->get_switchtype(lowerLayerSwitchingType));
		    cout<<"lowerLayerSwitchingType="<<this->get_switchtype(lowerLayerSwitchingType)<<endl;

		    lowerLayerEncodingType = (*iacdVar)->lowerLayerEncodingType;
		    pri_type_encoder_ptr->encodeString(PCE_LOWERLAYERENCODINGTYPE, this->get_encodetype(lowerLayerEncodingType));
		    cout<<"lowerLayerEncodingType="<<this->get_encodetype(lowerLayerEncodingType)<<endl;

		    upperLayerSwitchingType = (*iacdVar)->upperLayerSwitchingType;
		    pri_type_encoder_ptr->encodeString(PCE_UPPERLAYERSWITCHINGTYPE, this->get_switchtype(upperLayerSwitchingType));
		    cout<<"upperLayerSwitchingType="<<this->get_switchtype(upperLayerSwitchingType)<<endl;

		    upperLayerEncodingType = (*iacdVar)->upperLayerEncodingType;
		    pri_type_encoder_ptr->encodeString(PCE_UPPERLAYERENCODINGTYPE, this->get_encodetype(upperLayerEncodingType));
		    cout<<"upperLayerEncodingType="<<this->get_encodetype(upperLayerEncodingType)<<endl;

		    maxAdaptBandwidth = (*iacdVar)->maxAdaptBandwidth;
		    pri_type_encoder_ptr->encodeLong(PCE_MAXADAPTBANDWIDTH,maxAdaptBandwidth);
		    cout<<"maxAdaptBandwidth="<<maxAdaptBandwidth<<endl;
		}


	}



	if(opti_flag==1)
	{
		BandwidthAvailabilityGraph* bag=NULL;
		time_t new_time = 0;
		//time_t last_time = 0;
		u_int64_t bandwidth;
		int bag_size;
		int counter=0;
		map<time_t, u_int64_t>::iterator it;

		bag=path_info->GetBAG();
		if (bag != NULL)
		{
    		map<time_t, u_int64_t> TBSF=bag->GetTBSF();

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
    				pri_type_encoder_ptr->encodeLong(PCE_OPT_BAG_BANDWIDTH, bandwidth);
    				pri_type_encoder_ptr->encodeInteger(PCE_OPT_BAG_STARTTIME, new_time);

    				cout<<"bag bandwith="<<bandwidth<<endl;
    				cout<<"bag starttime="<<new_time<<endl;
    			}

    			//last_time = new_time;
    			counter++;
			}
		}

		//encode the last endtime for bag segment
		//pri_type_encoder_ptr->encodeInteger(PCE_OPT_BAG_ENDTIME, last_time);
		//cout<<"bag endtime="<<last_time<<endl;

	}

	pri_type_encoder_ptr->encodeString(PCE_PATH_END_TAG, "pathend");

}

string Apireplymsg_encoder::get_switchtype(u_char switchingType)
{
	string swit_type="";
	switch (switchingType)
	{
	case LINK_IFSWCAP_L2SC:
	{
		swit_type="l2sc";
		break;
	}
	case LINK_IFSWCAP_PSC1:
	{
		swit_type="psc-1";
		break;
	}
	case LINK_IFSWCAP_TDM:
	{
		swit_type="tdm";
		break;
	}
	case LINK_IFSWCAP_LSC:
	{
		swit_type="lsc";
		break;
	}
	default:
		break;
	}

	return swit_type;
}

string Apireplymsg_encoder::get_encodetype(u_char encodingType)
{
	string enco_type="";
	switch (encodingType)
	{
	case LINK_IFSWCAP_ENC_PKT:
		enco_type="packet";
		break;
	case LINK_IFSWCAP_ENC_ETH:
		enco_type="ethernet";
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
		enco_type="lambda";
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

	return enco_type;
}
