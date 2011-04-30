#ifndef _decode_pri_hh_
#define _decode_pri_hh_

#include <iostream>
#include <string>
#include <fstream>
#include "types.hh"

using namespace std;



class Decode_Pri_Type
{
public:

	int getLen(char* &, int &);
	bool decodeBoolean(char* &, int);
	int decodeInt(char* &, int);
	string decodeStr(char* &, int);
	float decodeFlo(char* &, int);
	double decodeDou(char* &, int);

};


#endif
