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
	u_int8_t pceType=PCE_REPLY;
	u_int16_t type;
	u_int16_t length;
	u_int8_t* value;

	ComputeResult* compute_result;

	string gri="";
	string err_msg="";
	TPath* path_info=NULL;
	list<TLink*> path;

	//TLink* link=NULL;
	ISCD* sw_cap_descriptors=NULL;
    u_char	switchingType;
    u_char	encodingType;
    ConstraintTagSet* availableVlanTags;
    ConstraintTagSet* suggestedVlanTags;
    ConstraintTagSet* assignedVlanTags;
    bool vlanTranslation;
    int mtu;
    int capacity;
    string rangStr="";

	char print_buff[200];

    memcpy(&compute_result, msg->GetTLVList().front()->value, sizeof(void*));

    cout<<"int="<<sizeof(int)<<" long="<<sizeof(long)<<endl;

	gri = compute_result->GetGri();
	this->gri_value = gri;
	cout<<"gri="<<gri<<endl;
	pri_type_encoder->encodeString(PCE_GRI, gri);
	err_msg = compute_result->GetErrMessage();
	cout<<"error msg="<<err_msg<<endl;
	path_info = compute_result->GetPathInfo();
	if(path_info == NULL)
	{
		cout<<"path info is null"<<endl;
	}
    if (path_info != NULL) 
    {
    	path = path_info->GetPath();

    	pri_type_encoder->encodeString(PCE_PATH_ID, "path-1");

    	cout<<"path length="<<path.size()<<endl;

    	pri_type_encoder->encodeInteger(PCE_PATH_LENGTH, path.size());

    	for(list<TLink*>::iterator it=path.begin();it!=path.end();it++)
    	{
    		cout<<"id="<<(*it)->GetId()<<endl;
    		cout<<"name="<<(*it)->GetName()<<endl;

    		pri_type_encoder->encodeString(PCE_LINK_ID, (*it)->GetName());//encode link id (name)

    		sw_cap_descriptors=(*it)->GetTheISCD();


    		cout<<"switchingtype="<<(int)sw_cap_descriptors->switchingType<<endl;
    		cout<<"encodingtype="<<(int)sw_cap_descriptors->encodingType<<endl;
    		cout<<"capacity="<<sw_cap_descriptors->capacity<<endl;

    		capacity = sw_cap_descriptors->capacity;

    		pri_type_encoder->encodeInteger(PCE_CAPACITY, capacity);

    		switchingType = sw_cap_descriptors->switchingType;
    		encodingType = sw_cap_descriptors->encodingType;

    	    switch (switchingType)
    	    {
    	        case LINK_IFSWCAP_L2SC:
    	        	cout<<"mtu="<<((ISCD_L2SC*)sw_cap_descriptors)->mtu<<endl;
    	        	cout<<"vlantranslation="<<((ISCD_L2SC*)sw_cap_descriptors)->vlanTranslation<<endl;

    	        	mtu = ((ISCD_L2SC*)sw_cap_descriptors)->mtu;

    	        	pri_type_encoder->encodeInteger(PCE_MTU, mtu);

    	        	vlanTranslation = ((ISCD_L2SC*)sw_cap_descriptors)->vlanTranslation;

    	        	pri_type_encoder->encodeBoolean(PCE_VLANTRANSLATION, vlanTranslation);

    	        	pri_type_encoder->encodeString(PCE_SWITCHINGCAPTYPE, "l2sc");

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
    	        		pri_type_encoder->encodeString(PCE_SWITCHINGVLANRANGEAVAI, rangStr);
    	        		cout<<"availableVlanTags="<<rangStr<<endl;
    	        	}
    	        	suggestedVlanTags = &((ISCD_L2SC*)sw_cap_descriptors)->suggestedVlanTags;
    	        	if(!suggestedVlanTags->IsEmpty())
    	        	{
    	        		rangStr=suggestedVlanTags->GetRangeString();
    	        		pri_type_encoder->encodeString(PCE_SWITCHINGVLANRANGESUGG, rangStr);
    	        		cout<<"suggestedVlanTags="<<rangStr<<endl;
    	        	}
    	        	assignedVlanTags = &((ISCD_L2SC*)sw_cap_descriptors)->assignedVlanTags;
    	        	if(!assignedVlanTags->IsEmpty())
    	        	{
    	        		rangStr=assignedVlanTags->GetRangeString();
    	        		pri_type_encoder->encodeString(PCE_SWITCHINGVLANRANGEASSI, rangStr);
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
    	            return NULL;
    	    }


    	    switch (encodingType)
    	    {
    	    	case LINK_IFSWCAP_ENC_PKT:
    	    		break;
    	    	case LINK_IFSWCAP_ENC_ETH:
    	    		pri_type_encoder->encodeString(PCE_SWITCHINGENCTYPE, "ethernet");
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
    }



    /*
	int a=2000;
	string b="test demo";
	float c=600.78;
	double d=10000.1234;

	pri_type_encoder->encodeInteger(PCE_GRI, a);
	pri_type_encoder->encodeString(PCE_TEST3, b);
	pri_type_encoder->encodeFloat(PCE_TEST1, c);
	pri_type_encoder->encodeDouble(PCE_TEST2, d);
    */

	msg_sub_ptr=pri_type_encoder->get_buff();
	msg_sublen=pri_type_encoder->get_length();

	msg_sub_start_ptr=encode_msg_sub_start(pceType, msg_sublen, msg_sub_startlen); //encode the subfield-header

	msg_body_ptr=encode_merge_buff(msg_sub_start_ptr, msg_sub_startlen, msg_sub_ptr, msg_sublen);  //merge subfield-header and body
	msg_len=msg_sub_startlen + msg_sublen; //body length

	delete[] msg_sub_start_ptr;
	delete[] msg_sub_ptr;

	body=(char*)msg_body_ptr;
	this->length=msg_len;

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
