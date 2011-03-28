#include "encode_pri.hh"

void Encode_Pri_Type::create_buff()
{
	buff = new u_int8_t[INIT_BUFF_SIZE];
	this->current_cap = INIT_BUFF_SIZE;
}

void Encode_Pri_Type::buff_rem_check(int req_size)
{
	int remain_size=0;
	remain_size=this->current_cap-offset;
	if(remain_size<=(req_size+8))
	{
		expand_buff();
	}
}

void Encode_Pri_Type::expand_buff()
{
	u_int8_t* buff_tmp = new u_int8_t[this->current_cap*2];
	memcpy(buff_tmp, buff, offset);
	delete[] buff;
	buff = buff_tmp;
	this->current_cap = this->current_cap*2;
}

void Encode_Pri_Type::buff_prune()
{
	u_int8_t* buff_tmp = new u_int8_t[offset];
	memcpy(buff_tmp, buff, offset);
	delete[] buff;
	buff = buff_tmp;
	this->current_cap = offset;
}

void Encode_Pri_Type::encodeHeader(u_int8_t pceType, u_int8_t priType, int length)
{
	buff[offset++] = pceType;
	//buff[offset++] = priType;

	if(length<0x80)
	{
		buff[offset++] = (u_int8_t)(length & 0xFf);
	}
	else if(length<=0xFF)
	{
		buff[offset++] = (0x01 | ASN_LONG_LEN);
		buff[offset++] = (u_int8_t)(length & 0xFF);
	}
	else if(length<=0xFFFF)
	{
		buff[offset++] = (0x02 | ASN_LONG_LEN);
		buff[offset++] = (u_int8_t)((length>>8) & 0xFF);
		buff[offset++] = (u_int8_t)(length & 0xFF);
	}
	else if(length<=0xFFFFFF)
	{
		buff[offset++] = (0x03 | ASN_LONG_LEN);
		buff[offset++] = (u_int8_t)((length>>16) & 0xFF);
		buff[offset++] = (u_int8_t)((length>>8) & 0xFF);
		buff[offset++] = (u_int8_t)(length & 0xFF);
	}
	else
	{

	}
}

void Encode_Pri_Type::encodeInteger(u_int8_t pcetype, int value)
{
	int int_val = value;
	int mask = 0;
	int intsize = 4;
	u_int8_t priType = INTEGER_NUM;

	mask = 0x1FF << ((8*3)-1);

	while((((int_val & mask)==0)||((int_val & mask)==mask)) && intsize>1)
	{
		intsize--;
		int_val <<= 8;

	}

	buff_rem_check(intsize+6);
	encodeHeader(pcetype, priType, intsize);
	mask = 0xFF << (8*3);
	while ((intsize--)>0)
	{
		buff[offset++] = (u_int8_t)((int_val & mask) >> (8 * 3));
		int_val <<= 8;
	}
}

void Encode_Pri_Type::encodeString(u_int8_t pceType, string value)
{
	string str = value;
	int strsize = 0;
	u_int8_t priType=CHARACTER_STRING;
	int i=0;
	strsize = str.length();

	buff_rem_check(strsize+6);
	encodeHeader(pceType, priType, strsize);

	for(i=0;i<strsize;i++)
	{
		buff[offset++] = str.at(i);
	}
}

void Encode_Pri_Type::encodeFloat(u_int8_t pceType, float value)
{
	float float_val=value;
	u_int8_t priType=FLOAT_NUM;
	int i=0;
	int floatsize=0;
	u_int8_t* byte_ptr;

	floatsize = sizeof(float);
	byte_ptr = (u_int8_t*)(&float_val);

	buff_rem_check(floatsize+6);
	encodeHeader(pceType, priType, floatsize);

	for(i=0;i<floatsize;i++)
	{
		buff[offset++] = byte_ptr[floatsize-1-i];
	}

}

void Encode_Pri_Type::encodeDouble(u_int8_t pceType, double value)
{
	double double_val=value;
	u_int8_t priType=DOUBLE_NUM;
	int i=0;
	int doublesize=0;
	u_int8_t* byte_ptr;

	doublesize = sizeof(double);
	byte_ptr = (u_int8_t*)(&double_val);

	buff_rem_check(doublesize+6);
	encodeHeader(pceType, priType, doublesize);

	for(i=0;i<doublesize;i++)
	{
		buff[offset++] = byte_ptr[doublesize-1-i];
	}

}
