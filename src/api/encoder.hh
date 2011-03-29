#ifndef ENCODER_HH_
#define ENCODER_HH_

#include "tag_id.hh"
#include "encode_pri.hh"
#include "apiserver.hh"

using namespace std;


class Apimsg_encoder
{
protected:
	Encode_Pri_Type* pri_type_encoder;
	int length;
	static int msg_seq_num;

	u_int8_t* encode_msg_sub_start(u_int8_t , int , int& );
	u_int8_t* encode_merge_buff(u_int8_t* , int , u_int8_t* , int );

public:
	Apimsg_encoder()
	{
		length=0;

		pri_type_encoder = new Encode_Pri_Type();
	}

	~Apimsg_encoder()
	{
		delete pri_type_encoder;
	}

	int test_encode_msg(Message*, char*&);

	void encode_msg_header(api_msg_header&, int);

};






#endif /* ENCODER_HH_ */
