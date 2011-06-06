package net.es.oscars.pce.tce;

public class EncodeApiMsgHeader {
	
	public static final int API_MSG_HEADER_LEN = 24;
	static int msg_seq_num = 1;
	
	EncodeApiMsgHeader(){
		//msg_seq_num = 1;
	}
	
	byte[] encoderApiMsg(short type, short length, int ucid, int options, int tag){
		byte[] header = new byte[API_MSG_HEADER_LEN];
		//byte byteVal = 0;
		int offset=0;
		
		fillField_16b(header, offset, type);
		offset = offset + 2;
		fillField_16b(header, offset, length);
		offset = offset + 2;
		fillField_32b(header, offset, ucid);
		offset = offset + 4;
		fillField_32b(header, offset, msg_seq_num++);
		offset = offset + 4;
		fillField_checksum(header, offset);
		offset = offset + 4;
		fillField_32b(header, offset, options);
		offset = offset + 4;
		fillField_32b(header, offset, tag);
		offset = offset + 4;
		
		return header;
		
	}
	
	void fillField_16b(byte[] header, int offset, short st_val){
		header[offset] = (byte)(0xFF & (st_val>>8));
		header[offset+1] = (byte)(0xFF & (st_val));
	
	}
	
	void fillField_32b(byte[] header, int offset, int int_val){
		for(int i=0;i<4;i++){
			header[offset+i] = (byte)(0xFF & (int_val >> 8*(3-i)));
		}
			
		
	}
	
	void fillField_checksum(byte[] header, int offset){
		long longval = 0;
		long longValue1 = 0;
		long longValue2 = 0;
		long longValue3 = 0;
		
		for(int i=0;i<4;i++){
			longval = longval << 8;
			//longval = longval | (0xFF & (long)header[i]);
			longval = longval | (0xFF & (long)header[3-i]);
			
		}
		longValue1 = longval;
		longval = 0;
		for(int i=4;i<8;i++){
			longval = longval << 8;
			//longval = longval | (0xFF & (long)header[i]);
			longval = longval | (0xFF & (long)header[11-i]);
			
		}
		longValue2 = longval;
		longval = 0;
		for(int i=8;i<12;i++){
			longval = longval << 8;
			//longval = longval | (0xFF & (long)header[i]);
			longval = longval | (0xFF & (long)header[19-i]);
			
		}
		longValue3 = longval;
		longval = 0;
		longval = longValue1 + longValue2 + longValue3;
		
		for(int i=0;i<4;i++){
			header[offset+3-i] = (byte)(0xFF & (longval >> 8*(3-i)));
		}
		
		//System.out.println("longval="+longval+"long1="+longValue1+"long2="+longValue2+"long3="+longValue3);
				
	}

}
