/*
 * reply_decoder.hh
 *
 *  Created on: May 18, 2011
 *      Author: wind
 */

#ifndef REPLY_DECODER_HH_
#define REPLY_DECODER_HH_

#include "tag_id.hh"
#include "decode_pri.hh"
#include "compute_result.hh"
#include "api.hh"
#include <string>

class Apireplymsg_decoder
{
protected:
	Decode_Pri_Type pri_type_decoder;

public:

	Compute_result* decode_reply_msg(char*, int);
	void decode_compute_result(char* &, int , Compute_result* );





};



#endif /* REPLY_DECODER_HH_ */
