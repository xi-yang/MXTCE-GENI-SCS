#ifndef ENCODER_HH_
#define ENCODER_HH_

#include "tag_id.hh"
#include "encode_pri.hh"
#include "apiserver.hh"

using namespace std;

class Apimsg_encoder
{
protected:
	Encode_Pri_Type pri_type_encoder;
	int length;

public:
	Apimsg_encoder()
	{
		length=0;
	}

	int test_encode_msg(Message*, char*);

	void encode_msg_header(api_msg_header&, int);

};





#endif /* ENCODER_HH_ */
