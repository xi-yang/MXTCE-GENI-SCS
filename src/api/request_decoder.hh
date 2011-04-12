#ifndef request_decoder_hh_
#define request_decoder_hh_

#include "tag_id.hh"
#include "decode_pri.hh"
#include "reserved_constraint.hh"
#include "user_constraint.hh"
#include <string>

using namespace std;


class Apireqmsg_decoder
{
protected:

	Decode_Pri_Type pri_type_decoder;


public:
	Apireqmsg_decoder(){}

	Apimsg_user_constraint* test_decode_msg(char*, int );
	void decode_usercons(char* , int , Apimsg_user_constraint* );
	void decode_resvcons(char* , int , Apimsg_user_constraint* );

};

#endif
