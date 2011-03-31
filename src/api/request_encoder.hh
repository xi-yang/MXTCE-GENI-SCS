#ifndef REQUEST_ENCODER_HH_
#define REQUEST_ENCODER_HH_

#include "tag_id.hh"
#include "encode_pri.hh"
#include "user_constraint.hh"
#include "api.hh"

using namespace std;

#define MSG_CHKSUM(X) (((u_int32_t*)&X)[0] + ((u_int32_t*)&X)[1] + ((u_int32_t*)&X)[2])

class Apireqmsg_encoder
{
protected:
	Encode_Pri_Type* pri_type_encoder;
	int length;
	static int msg_seq_num;
	Apimsg_user_constraint* user_constraint;


	u_int8_t* encode_msg_sub_start(u_int8_t , int , int& );
	u_int8_t* encode_merge_buff(u_int8_t* , int , u_int8_t* , int );
	void encode_msg_header(api_msg_header*, int);

public:
	Apireqmsg_encoder()
	{
		length=0;
		pri_type_encoder = new Encode_Pri_Type();
	}

	~Apireqmsg_encoder()
	{
		delete pri_type_encoder;
	}

	api_msg* test_encode_msg(Apimsg_user_constraint* );



};






#endif /* REQUEST_ENCODER_HH_ */
