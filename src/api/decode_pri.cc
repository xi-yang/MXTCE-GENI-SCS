
#include "decode_pri.hh"
#include "log.hh"


int Decode_Pri_Type::getLen(char* & decode_ptr, int & len_tag_len)
{
    //u_int8_t length=0;
    u_int8_t bytevalue=0;
    u_int8_t length_field_val=0;
    u_int8_t length_field_len=0;
    int intvalue=0;
	int length=0;
	
   

    memcpy(&length_field_val, decode_ptr++, sizeof(char));

     if((length_field_val & 0x80)>0)
     {
         length_field_len=length_field_val & 0x7F;
         len_tag_len=length_field_len+1;
         while(length_field_len-- >0)
         {
             memcpy(&bytevalue, decode_ptr++, sizeof(char));
             intvalue=bytevalue & 0xFF;
             length=(length<<8) | intvalue;
         }
      }
     else
     {
    	 len_tag_len=1;
    	 length=length_field_val & 0xFF;
     }


     return length;
}

bool Decode_Pri_Type::decodeBoolean(char* & decode_ptr, int length)
{
	bool result = 0;
	u_int8_t bytevalue=0;

	memcpy(&bytevalue, decode_ptr++, sizeof(char));

	if(bytevalue==0)
	{
		result = false;
	}
	else
	{
		result = true;
	}

	return result;

}

int Decode_Pri_Type::decodeInt(char* & decode_ptr, int length)
{
	int intvalue = 0;
	int result = 0;
	u_int8_t bytevalue=0;

	memcpy(&bytevalue, decode_ptr++, sizeof(char));
	
	intvalue = bytevalue & 0xFF;
    if((intvalue & 0x80)>0)
	{
		result = -1;  //result is negative, use -1 (all bit set to 1) as initial value to shift left
	}

	while(length-- >0)
	{
		result = (result<<8) | intvalue;

		if(length>0)
		{
			memcpy(&bytevalue, decode_ptr++, sizeof(char));
			intvalue = bytevalue & 0xFF;
		}
	}

	return result;

}

u_int64_t Decode_Pri_Type::decodeLong(char* & decode_ptr, int length)
{
	u_int64_t longvalue = 0L;
	u_int64_t result = 0L;
	u_int8_t bytevalue=0;

	memcpy(&bytevalue, decode_ptr++, sizeof(char));

	longvalue = bytevalue & 0xFF;
    if((longvalue & 0x80L)>0)
	{
		result = -1;  //result is negative, use -1 (all bit set to 1) as initial value to shift left
	}

	while(length-- >0)
	{
		result = (result<<8) | longvalue;

		if(length>0)
		{
			memcpy(&bytevalue, decode_ptr++, sizeof(char));
			longvalue = bytevalue & 0xFF;
		}
	}

	return result;

}

string Decode_Pri_Type::decodeStr(char* & decode_ptr, int length)
{
	char charvalue = 0;
	string result = "";

	while(length-- >0)
	{
		memcpy(&charvalue, decode_ptr++, sizeof(char));
		result = result + charvalue;
	}

	return result;
}


float Decode_Pri_Type::decodeFlo(char* & decode_ptr, int length)
{
	int i=0;
	char charvalue = 0;
	char charvalarr[4];
	float result = 0;
	float *flo_ptr;
	//char *cha_ptr;

	for(i=0;i<4;i++)
	{
		memcpy(&charvalue, decode_ptr++, sizeof(char));
		charvalarr[3-i]=charvalue;
	}

	//cha_ptr=charvalarr;
	flo_ptr=(float *)charvalarr;

	result=*flo_ptr;
		

	//infile.read((char *)&result, sizeof(float));

	return result;
}


double Decode_Pri_Type::decodeDou(char* & decode_ptr, int length)
{
	int i=0;
	char charvalue=0;
	char charvalarr[8];
	double result = 0;
	double *dou_ptr;

	for(i=0;i<8;i++)
	{
		memcpy(&charvalue, decode_ptr++, sizeof(char));
		charvalarr[7-i]=charvalue;
	}

	dou_ptr=(double *)charvalarr;

	result=*dou_ptr;
	
	//infile.read((char *)&result, sizeof(double));

	return result;
}



