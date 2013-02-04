#ifndef REPLY_ENCODER_HH_
#define REPLY_ENCODER_HH_

#include "tag_id.hh"
#include "encode_pri.hh"
#include "apiserver.hh"
#include "compute_worker.hh"
#include "scheduling.hh"

using namespace std;


class Apireplymsg_encoder
{
protected:
	Encode_Pri_Type* pri_type_encoder;
	int length;
	static int msg_seq_num;
	int path_seq_num;
	int alt_path_seq_num;

	string gri_value;

	u_int8_t* encode_msg_sub_start(u_int8_t , int , int& );
	u_int8_t* encode_merge_buff(u_int8_t* , int , u_int8_t* , int );

public:
	Apireplymsg_encoder()
	{
		length=0;
		path_seq_num=0;
		alt_path_seq_num=0;

		pri_type_encoder = new Encode_Pri_Type();
	}

	~Apireplymsg_encoder()
	{
		delete pri_type_encoder;
	}

	int test_encode_msg(ComputeResult*, char*&);

	void encode_msg_header(api_msg_header&, int);

	void encode_path(TPath*, string, Encode_Pri_Type*, int);

	string get_gri()
	{
		return this->gri_value;
	}

	string get_switchtype(u_char);
	string get_encodetype(u_char);

};






#endif /* REPLY_ENCODER_HH_ */
