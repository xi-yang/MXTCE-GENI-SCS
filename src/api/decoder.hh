#ifndef _decoder_hh_
#define _decoder_hh_

#include "tag_id.hh"
#include "decode_pri.hh"
//#include "reserved_constraint.hh"
#include "user_constraint.hh"
#include <string>

using namespace std;


class Apimsg_decoder
{
protected:

	Decode_Pri_Type pri_type_decoder;


public:
	Apimsg_decoder(){}

	Apimsg_user_constraint* test_decode_msg(char* );
	void decode_usercons(char* , int , Apimsg_user_constraint* );

};

#endif
