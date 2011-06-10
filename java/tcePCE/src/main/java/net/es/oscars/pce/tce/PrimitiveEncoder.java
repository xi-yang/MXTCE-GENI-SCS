package net.es.oscars.pce.tce;

import java.io.IOException;
import java.io.OutputStream;

public class PrimitiveEncoder {
	
	byte buff[];
	int offset;
	
	public static final int initBuffSize = 32;

	public static final byte ASN_LONG_LEN = (byte)0x80;
	
	public static final byte BOOLEAN = (byte)0x01;
	public static final byte INTEGER_NUM = (byte)0x02;
	public static final byte ENUMERATED_NUM = (byte)0x03;
	public static final byte FLOAT_NUM = (byte)0x04;
	public static final byte DOUBLE_NUM = (byte)0x05;
	public static final byte CHARACTER_STRING = (byte)0x06;
	
	public PrimitiveEncoder(){
		this.createBuff();
		this.offset = 0;
	}
	
	public void createBuff(){
		buff = new byte[initBuffSize];
	}
	
	public void recreateBuff(){
		buff = new byte[initBuffSize];
	}
	
	public void expandBuff(int reqSize){
		int length = buff.length;
		int size;
		if(reqSize <= length){
			size = length + length;
		}
		else{
			size = length + reqSize;
		}
		byte[] newBuff = new byte[size];
		
		System.arraycopy(buff, 0, newBuff, 0, length);
		buff = newBuff;
	}
	
	public void buffRemCheck(int reqSize){
		int remainSize;
		remainSize=buff.length - offset;
		if(remainSize<=(reqSize+8)){
			this.expandBuff(reqSize+8-remainSize);
		}
	}
	
	public void buffPrune(){
		byte newBuff[] = new byte[offset];
		System.arraycopy(buff, 0, newBuff, 0, offset);
		buff = newBuff;
	}
	
	public byte[] getBuff(){
		return buff;
	}
	
	
	
	public void encodeHeader(byte pceType, byte priType, int length){
		buff[offset++] = pceType;
		//buff[offset++] = priType;
		
		if(length<0x80){
			buff[offset++] = (byte) (length & 0xFF);			
		}
		else if(length<=0xFF){
			buff[offset++] = (0x01 | ASN_LONG_LEN);
			buff[offset++] = (byte) (length & 0xFF);			
		}
		else if (length<=0xFFFF){
			buff[offset++] = (0x02 | ASN_LONG_LEN);
			buff[offset++] = (byte) ((length>>8) & 0xFF);
			buff[offset++] = (byte) (length & 0xFF);
		}
		else if (length<=0xFFFFFF){
			buff[offset++] = (0x03 | ASN_LONG_LEN);
			buff[offset++] = (byte) ((length>>16) & 0xFF);
			buff[offset++] = (byte) ((length>>8) & 0xFF);
			buff[offset++] = (byte) (length & 0xFF);
		}
		else{
			
		}
		
	}
	
	public void encodeBoolean(byte pceType, boolean value){
		boolean boolVal = value;
		int boolSize = 1;
		byte priType = BOOLEAN;
		
		buffRemCheck(boolSize+6);
		encodeHeader(pceType, priType, boolSize);
		if(boolVal==true){
			buff[offset++] = (byte) 0x01;
		}else{
			buff[offset++] = (byte) 0x00;
		}
	}
	
	public void encodeInteger(byte pceType, int value){
		int integer = value;
		int mask;
		int intsize = 4;
		byte priType = INTEGER_NUM;


		mask = 0x1FF << ((8 * 3) - 1);
		
		while((((integer & mask) == 0) || ((integer & mask) == mask))
				&& intsize > 1){
			intsize--;
			integer <<= 8;
			}
		buffRemCheck(intsize+6);
		encodeHeader(pceType, priType, intsize);
		mask = 0xFF << (8 * 3);
		
		while ((intsize--) > 0){
			
			buff[offset++] = (byte) ((integer & mask) >> (8 * 3));
			integer <<= 8;
			}
		}
	
	public void encodeLong(byte pceType, long value){
		long longInt = value;
		long mask;
		int intsize = 8;
		byte priType = INTEGER_NUM;


		mask = ((long)0x1FF) << ((8 * 7) - 1);
		
		
		while((((longInt & mask) == 0) || ((longInt & mask) == mask))
				&& intsize > 1){
			intsize--;
			longInt <<= 8;
			}
		
		buffRemCheck(intsize+6);
		encodeHeader(pceType, priType, intsize);
		mask = 0xFF << (8 * 7);		
		while ((intsize--) > 0){
			
			buff[offset++] = (byte) ((longInt & mask) >> (8 * 7));
			longInt <<= 8;
			}
		}
	
	public void encodeString(byte pceType, String value){
		String str = value;
		byte[] ascStr = null;
		int strsize = 0;
		byte priType = CHARACTER_STRING;
		
		try{
			
			ascStr = str.getBytes("US-ASCII");
			
		}
		catch(java.io.UnsupportedEncodingException e){
			System.out.println("unsupported encoding exception");
			e.printStackTrace(); 
		}
		strsize = ascStr.length;
		
		buffRemCheck(strsize+6);
		encodeHeader(pceType, priType, strsize);
		
		
		for(int i=0; i<strsize; i++){
			buff[offset++]=ascStr[i];
		}	
		
	}
	
	public void encodeDouble(byte pceType, double value){
		double doubleNum = value;
		int doublesize;
		byte priType = DOUBLE_NUM;
		long doubleEncode;
		int arrSize;
		byte[] doubleByte;
		
		doublesize = Double.SIZE/8;
		
		buffRemCheck(doublesize+6);
		encodeHeader(pceType, priType, doublesize);
		doubleEncode = Double.doubleToLongBits(doubleNum);
		arrSize = Double.SIZE / Byte.SIZE;
		doubleByte = new byte[arrSize];
		for(int i=0; i<arrSize; i++){
			doubleByte[arrSize-i-1] = (byte) ((doubleEncode>>(i*8))&0xFF);
		}
		for(int i=0; i<arrSize; i++){
			buff[offset++] = doubleByte[i];
		}

	}
	
	public void encodeFloat(byte pceType, float value){
		float floatNum = value;
		int floatsize;
		byte priType = FLOAT_NUM;
		int floatEncode;
		int arrSize;
		byte[] floatByte;
		
		floatsize = Float.SIZE/8;
		
		buffRemCheck(floatsize+6);
		encodeHeader(pceType, priType, floatsize);
		floatEncode = Float.floatToIntBits(floatNum);
		arrSize = Float.SIZE / Byte.SIZE;
		floatByte = new byte[arrSize];
		for(int i=0; i<arrSize; i++){
			floatByte[arrSize-i-1] = (byte) ((floatEncode>>(i*8))&0xFF);
		}
		for(int i=0; i<arrSize; i++){
			buff[offset++] = floatByte[i];
		}

	}

}
