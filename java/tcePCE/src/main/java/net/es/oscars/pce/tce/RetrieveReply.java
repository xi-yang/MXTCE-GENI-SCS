package net.es.oscars.pce.tce;

import java.util.ArrayList;
import java.util.List;

import net.es.oscars.utils.soap.OSCARSServiceException;

public class RetrieveReply {
	
	byte[] receivedApiMsg;
	int offset=0;
	
	
	RetrieveReply(){
		
	}
	
	public void checkApiMsg(byte[] apiMsg) throws OSCARSServiceException{
		int type = this.getTwoByte(apiMsg);
		int length = this.getTwoByte(apiMsg);
		int ucid = this.getFourByte(apiMsg);
		int sequenceNum = this.getFourByte(apiMsg);
		//System.out.println("type="+type+" length="+length+" ucid="+ucid+" sequenceNumber="+sequenceNum);
		int checkSum = this.getFourByte(apiMsg);
		int option = this.getFourByte(apiMsg);
		int tag = this.getFourByte(apiMsg);
		
		boolean checkSumValid = this.verifyCheckSum(type, length, ucid, sequenceNum, checkSum);
		
		if(checkSumValid == false){
			//System.out.println("Checksum error");
			throw new OSCARSServiceException("Checksum of API message error");
		}
		
		//this.decodeReplyMessage(apiMsg);
		
		
	}
	
	private boolean verifyCheckSum(int type, int length, int ucid, int seqNum, int checkSum){
		boolean valid = true;
		byte byteValue = 0;
		long value = 0;
		long fieldOne = 0;
		long fieldTwo = 0;
		long fieldThree = 0;
		long sum = 0;
		long checkSumValue = 0;
		
		for(int i=0;i<2;i++){
			byteValue = (byte) (0xFF & length);
			value = ((long)0xFF) & byteValue;
			fieldOne = (fieldOne << 8) | value;
			length = length >> 8;			
		}
		//System.out.println("v1 a="+fieldOne);
		
		for(int i=0;i<2;i++){
			byteValue = (byte) (0xFF & type);
			value = ((long)0xFF) & byteValue;
			fieldOne = (fieldOne << 8) | value;
			type = type >> 8;
		}
		//System.out.println("v1 ="+fieldOne);
		
		for(int i=0;i<4;i++){
			byteValue = (byte) (0xFF & ucid);
			value = ((long)0xFF) & byteValue;
			fieldTwo = (fieldTwo << 8) | value;
			ucid = ucid >> 8;			
		}
		//System.out.println("v2="+fieldTwo);
		
		for(int i=0;i<4;i++){
			byteValue = (byte) (0xFF & seqNum);
			value = ((long)0xFF) & byteValue;
			fieldThree = (fieldThree << 8) | value;
			seqNum = seqNum >> 8;			
		}
		//System.out.println("v3="+fieldThree);
		
		sum = fieldOne + fieldTwo + fieldThree;
		sum = sum & ((long) 0xFFFFFFFF);
		//checkSumValue = checkSum & ((long) 0xFFFFFFFF);
		
		//System.out.println("checksum="+checkSum);
		
		for(int i=0;i<4;i++){
			byteValue = (byte) (0xFF & checkSum);
			value = ((long)0xFF) & byteValue;
			checkSumValue = (checkSumValue << 8) | value;
			checkSum = checkSum >> 8;
		}
		
		//System.out.println("checksum ori="+checkSumValue+" sum="+sum);
		
		if(sum == checkSumValue){
			valid = true;
		}else{
			valid = false;
		}	
		
		return valid;
	}
	
	private int getTwoByte(byte[] buff){
		int result = 0;
		int value = 0;
		
		for(int i=0;i<2;i++){
			value = 0xFF & buff[offset++];
			result = (result << 8) | value;
		}
		
		return result;
	}
	
	private int getFourByte(byte[] buff){
		int result = 0;
		int value = 0;
		
		for(int i=0;i<4;i++){
			value = 0xFF & buff[offset++];
			result = (result<<8) | value;
		}
		
		return result;
	}
	
	public ReplyMessageContent decodeReplyMessage(byte[] buff){
		PrimitiveDecoder priDecoder = new PrimitiveDecoder();
		byte type = 0;
		int lengthTagSize = 0;
		int length = 0;
		ReplyMessageContent replyMessage = new ReplyMessageContent();
		
		type = buff[offset++];
		
		if(type == CodeNumber.PCE_REPLY){
			lengthTagSize = priDecoder.getLengthTagSize(buff, offset);
			length = priDecoder.getLength(buff, offset);
			offset = offset + lengthTagSize;
			this.decodeReplyPathContent(buff, length, replyMessage);
		}
			
		//System.out.println("test-2");
		
		return replyMessage;
		
		
	}
	
	void decodeReplyPathContent(byte[] buff, int totalLength, ReplyMessageContent replyMessage){
		PrimitiveDecoder priDecoder = new PrimitiveDecoder();
		byte type = 0;
		int lengthTagSize = 0;
		int length = 0;
		String gri;
		String errorMessage;
		String pathId;
		int pathLength = 0;
		String linkName;
		String switchingCapType;
		String switchingEncType;
		String assignedVlanTags;
		String suggestedVlanTags;
		String availableVlanTags;
		boolean vlanTranslation;
		int capacity;
		int mtu;
		String remoteLinkId;
		int maximumReservableCapacity;
		int minimumReservableCapacity;
		int granularity;
		int trafficEngineeringMetric;
		ReplyLinkContent replyLink = null;
		List<ReplyLinkContent> linkSet = null;
		int initialDecodeOffset = 0;
		
		initialDecodeOffset = this.offset;		
		
		type = buff[offset++];
		if(type == CodeNumber.PCE_GRI){
			lengthTagSize = priDecoder.getLengthTagSize(buff, offset);
			length = priDecoder.getLength(buff, offset);
			offset = offset + lengthTagSize;
			gri = priDecoder.decodeString(buff, offset, length);
			//System.out.println("gri="+gri);
			
			offset = offset + length;
			replyMessage.setGri(gri);
		}
		
		type = buff[offset++];
		if(type==CodeNumber.PCE_COMPUTE_ERROR){
			length = this.decodeLength(priDecoder, buff);
			errorMessage = priDecoder.decodeString(buff, offset, length);
			//System.out.println("errormsg="+errorMessage);
			replyMessage.setErrorMessage(errorMessage);
			return;
		}
		
		if(type == CodeNumber.PCE_PATH_ID){
			length = this.decodeLength(priDecoder, buff);
			pathId = priDecoder.decodeString(buff, offset, length);
			//System.out.println("pathid="+pathId);
			
			offset = offset + length;
			ReplyPathContent replyPath = new ReplyPathContent();
			replyMessage.setReplyPathContent(replyPath);  //set replyPathContent to object variable
			replyMessage.getReplyPathContent().setId(pathId);
			
			type = buff[offset++];			
			if(type == CodeNumber.PCE_PATH_LENGTH){
				length = this.decodeLength(priDecoder, buff);
				pathLength = priDecoder.decodeInteger(buff, offset, length);
				//System.out.println("pathlength="+pathLength);
				
				offset = offset + length;
				
			}
			
			linkSet = replyMessage.getReplyPathContent().getReplyLinkContent();
			
			while((this.offset - initialDecodeOffset) < totalLength){
				type = buff[offset++];
				if(type == CodeNumber.PCE_LINK_ID){
					if(replyLink != null){
						linkSet.add(replyLink);
					}
					replyLink = new ReplyLinkContent();
					length = this.decodeLength(priDecoder, buff);
					linkName = priDecoder.decodeString(buff, offset, length);
					//System.out.println("linkname="+linkName);
					
					offset = offset + length;
					
					replyLink.setName(linkName);
								
				}else if(type == CodeNumber.PCE_REMOTE_LINK){
					length = this.decodeLength(priDecoder, buff);
					remoteLinkId = priDecoder.decodeString(buff, offset, length);
					
					offset = offset + length;
					
					replyLink.setRemoteLinkId(remoteLinkId);
					
				}else if(type == CodeNumber.PCE_SWITCHINGCAPTYPE){
					length = this.decodeLength(priDecoder, buff);
					switchingCapType = priDecoder.decodeString(buff, offset, length);
					//System.out.println("switchingcaptype="+switchingCapType);
					
					offset = offset + length;
					
					replyLink.setSwitchingType(switchingCapType);					
				}else if(type == CodeNumber.PCE_SWITCHINGENCTYPE){
					length = this.decodeLength(priDecoder, buff);
					switchingEncType = priDecoder.decodeString(buff, offset, length);
					//System.out.println("switchingenctype="+switchingEncType);
					
					offset = offset + length;
					
					replyLink.setEncodingType(switchingEncType);
				}else if(type == CodeNumber.PCE_SWITCHINGVLANRANGEAVAI){
					length = this.decodeLength(priDecoder, buff);
					availableVlanTags = priDecoder.decodeString(buff, offset, length);
					//System.out.println("availablevlantags="+availableVlanTags);
					
					offset = offset + length;
					
					replyLink.setAvailableVlanTags(availableVlanTags);
				}else if(type == CodeNumber.PCE_SWITCHINGVLANRANGESUGG){
					length = this.decodeLength(priDecoder, buff);
					suggestedVlanTags = priDecoder.decodeString(buff, offset, length);
					//System.out.println("suggestedvlantags="+suggestedVlanTags);
					
					offset = offset + length;
					
					replyLink.setSuggestedVlanTags(suggestedVlanTags);
				}else if(type == CodeNumber.PCE_SWITCHINGVLANRANGEASSI){
					length = this.decodeLength(priDecoder, buff);
					assignedVlanTags = priDecoder.decodeString(buff, offset, length);
					//System.out.println("assignedvlantags="+assignedVlanTags);
					
					offset = offset + length;
					
					replyLink.setAssignedVlanTags(assignedVlanTags);
				}else if(type == CodeNumber.PCE_VLANTRANSLATION){
					length = this.decodeLength(priDecoder, buff);
					vlanTranslation = priDecoder.decodeBoolean(buff, offset, length);
					//System.out.println("vlantranslation="+vlanTranslation);
					
					offset = offset + length;
					
					replyLink.setVlanTranslation(vlanTranslation);
				}else if(type == CodeNumber.PCE_CAPACITY){
					length = this.decodeLength(priDecoder, buff);
					capacity = priDecoder.decodeInteger(buff, offset, length);
					//System.out.println("capacity="+capacity);
					
					offset = offset + length;
					
					replyLink.setCapacity(capacity);
				}else if(type == CodeNumber.PCE_MTU){
					length = this.decodeLength(priDecoder, buff);
					mtu = priDecoder.decodeInteger(buff, offset, length);
					//System.out.println("mtu="+mtu);
					
					offset = offset + length;
					
					replyLink.setMtu(mtu);
				}else if(type == CodeNumber.PCE_MAXRESVCAPACITY){
					length = this.decodeLength(priDecoder, buff);
					maximumReservableCapacity = priDecoder.decodeInteger(buff, offset, length);
					
					offset = offset + length;
					
					replyLink.setMaximumReservableCapacity(maximumReservableCapacity);					
				}else if(type == CodeNumber.PCE_MINRESVCAPACITY){
					length = this.decodeLength(priDecoder, buff);
					minimumReservableCapacity = priDecoder.decodeInteger(buff, offset, length);
					
					offset = offset + length;
					
					replyLink.setMinimumReservableCapacity(minimumReservableCapacity);
				}else if(type == CodeNumber.PCE_GRANULARITY){
					length = this.decodeLength(priDecoder, buff);
					granularity = priDecoder.decodeInteger(buff, offset, length);
					
					offset = offset + length;
					
					replyLink.setGranularity(granularity);
				}else if(type == CodeNumber.PCE_TE_METRIC){
					length = this.decodeLength(priDecoder, buff);
					trafficEngineeringMetric = priDecoder.decodeInteger(buff, offset, length);
					
					offset = offset + length;
					
					replyLink.setTrafficEngineeringMetric(trafficEngineeringMetric);
				}else{
					
				}
				
			}
			linkSet.add(replyLink); //last link			
			
		}
		//System.out.println("test-1");
		
	}
	
	int decodeLength(PrimitiveDecoder priDecoder, byte[] buff){
		int lengthTagSize = 0;
		int length = 0;
		lengthTagSize = priDecoder.getLengthTagSize(buff, offset);
		length = priDecoder.getLength(buff, offset);
		offset = offset + lengthTagSize;
		return length;
	}

}
