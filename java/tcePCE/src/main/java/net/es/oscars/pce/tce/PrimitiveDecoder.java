package net.es.oscars.pce.tce;

public class PrimitiveDecoder {
	
	
	public int getLength(byte[] buff, int offset){
		byte byteValue = 0;
		byte lengthFieldValue = 0;
		byte lengthFieldLength = 0;
		
		
		int intValue = 0;
		int length = 0;
		
		lengthFieldValue = buff[offset++]; //read tag field value
		if((lengthFieldValue & 0x80)>0){
			lengthFieldLength = (byte) (lengthFieldValue & 0x7F);
			while(lengthFieldLength-- >0){
				byteValue = buff[offset++];
				intValue = byteValue & 0xFF;
				length = (length<<8) | intValue;
			}
		}else{
			length = lengthFieldValue & 0xFF;
		}
		
		return length;
		
	}
	
	public int getLengthTagSize(byte[] buff, int offset){
		byte lengthFieldValue = 0;
		byte lengthFieldLength = 0;
		int lengthTagSize = 0;
		
		lengthFieldValue = buff[offset++]; //read tag field value
		if((lengthFieldValue & 0x80)>0){
			lengthFieldLength = (byte) (lengthFieldValue & 0x7F);
			lengthTagSize = lengthFieldLength + 1;
		}else{
			lengthTagSize = 1;			
		}
		
		return lengthTagSize;
		
	}
	
	public boolean decodeBoolean(byte[] buff, int offset, int length){
		boolean result = false;
		byte byteValue = 0;
		
		byteValue = buff[offset++];
		if(byteValue==0){
			result = false;
		}else{
			result = true;
		}
		
		return result;
	}
	
	
	int decodeInteger(byte[] buff, int offset, int length){
		byte byteValue = 0;
		int intValue = 0;
		int result = 0;
		
		byteValue = buff[offset++];
		intValue = byteValue & 0xFF;
		if((intValue & 0x80) > 0){
			result = -1;  //negative value, use -1 (all bits set to 1) to initial value for shift left
		}
		
		while(length-- >0){
			result = (result<<8) | intValue;
			
			if(length > 0){
				byteValue = buff[offset++];
				intValue = byteValue & 0xFF;
			}
		}
		
		return result;
		
	}
	
	long decodeLong(byte[] buff, int offset, int length){
		byte byteValue = 0;
		long longValue = 0L;
		long result = 0L;
		
		byteValue = buff[offset++];
		longValue = byteValue & 0xFF;
		if((longValue & 0x80L) > 0){
			result = -1;  //negative value, use -1 (all bits set to 1) to initial value for shift left
		}
		
		while(length-- >0){
			result = (result<<8) | longValue;
			
			if(length > 0){
				byteValue = buff[offset++];
				longValue = byteValue & 0xFF;
			}
		}
		
		return result;
		
	}

	
	String decodeString(byte[] buff, int offset, int length){
		String result = new String(buff, offset, length);
		offset = offset + length;
		return result;
	}
	
	float decodeFloat(byte[] buff, int offset, int length){
		int floatBit = 0;
		byte byteValue = 0;
		int intValue = 0;
		float result = 0;
		
		while(length-- >0){
			byteValue = buff[offset++];
			intValue = byteValue & 0xFF;
			floatBit = (floatBit<<8) | intValue;
		}
		result = Float.intBitsToFloat(floatBit);
		
		return result;
		
	}
	
	double decodeDouble(byte[] buff, int offset, int length){
		long doubleBit = 0;
		byte byteValue = 0;
		long longValue = 0;
		double result = 0;
		
		while(length-- >0){
			byteValue = buff[offset++];
			longValue = byteValue & 0xFF;
			doubleBit =- (doubleBit<<8) | longValue;
		}
		result = Double.longBitsToDouble(doubleBit);
		
		return result;
		
	}


}
