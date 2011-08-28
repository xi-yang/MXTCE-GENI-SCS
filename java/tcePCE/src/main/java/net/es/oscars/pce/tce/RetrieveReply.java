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
		
		int checkSum = this.getFourByte(apiMsg);
		int option = this.getFourByte(apiMsg);
		int tag = this.getFourByte(apiMsg);
		
		boolean checkSumValid = this.verifyCheckSum(type, length, ucid, sequenceNum, checkSum);
		
		if(checkSumValid == false){
			
			throw new OSCARSServiceException("Checksum of API message error");
		}
		
		
		
		
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
		
		
		for(int i=0;i<2;i++){
			byteValue = (byte) (0xFF & type);
			value = ((long)0xFF) & byteValue;
			fieldOne = (fieldOne << 8) | value;
			type = type >> 8;
		}
		
		
		for(int i=0;i<4;i++){
			byteValue = (byte) (0xFF & ucid);
			value = ((long)0xFF) & byteValue;
			fieldTwo = (fieldTwo << 8) | value;
			ucid = ucid >> 8;			
		}
		
		
		for(int i=0;i<4;i++){
			byteValue = (byte) (0xFF & seqNum);
			value = ((long)0xFF) & byteValue;
			fieldThree = (fieldThree << 8) | value;
			seqNum = seqNum >> 8;			
		}
		
		
		sum = fieldOne + fieldTwo + fieldThree;
		sum = sum & ((long) 0xFFFFFFFF);
		
		
		
		
		for(int i=0;i<4;i++){
			byteValue = (byte) (0xFF & checkSum);
			value = ((long)0xFF) & byteValue;
			checkSumValue = (checkSumValue << 8) | value;
			checkSum = checkSum >> 8;
		}
		
		
		
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
	
	public ReplyMessageContent decodeReplyMessage(byte[] buff) throws OSCARSServiceException{
		PrimitiveDecoder priDecoder = new PrimitiveDecoder();
		byte type = 0;
		int lengthTagSize = 0;
		int totalMsgLength = 0;
		int regFieldLength = 0;
		int optFieldLength = 0;
		int decodedLength = 0;
		int length = 0;
		int initialDecodeOffset = 0;
		String gri;
		String errorMessage;
		ReplyMessageContent replyMessage = new ReplyMessageContent();
		
		type = buff[offset++];
		
		if(type == CodeNumber.PCE_REPLY){
			lengthTagSize = priDecoder.getLengthTagSize(buff, offset);
			totalMsgLength = priDecoder.getLength(buff, offset);
			offset = offset + lengthTagSize;
			
			initialDecodeOffset = offset;
			
			type = buff[offset++];

			if(type == CodeNumber.PCE_GRI){
				length = this.decodeLength(priDecoder, buff);
				gri = priDecoder.decodeString(buff, offset, length);
							
				offset = offset + length;
				replyMessage.setGri(gri);
			}
			
			type = buff[offset++];
			if(type==CodeNumber.PCE_COMPUTE_ERROR){
				length = this.decodeLength(priDecoder, buff);
				errorMessage = priDecoder.decodeString(buff, offset, length);
				
				offset = offset + length;			
				replyMessage.setErrorMessage(errorMessage);
				return replyMessage;
			}
			
			if(type == CodeNumber.PCE_REGU_REPLY){
				regFieldLength = this.decodeLength(priDecoder, buff);
				this.decodeReplyPathContent(buff, regFieldLength, replyMessage);
				decodedLength = offset - initialDecodeOffset;
				
				if(decodedLength < totalMsgLength){
					type = buff[offset++];
					if(type == CodeNumber.PCE_OPTI_REPLY){
						optFieldLength = this.decodeLength(priDecoder, buff);
						this.decodeOptiContent(buff, replyMessage);
						
												
					}
				}				
			}			
		}
			
		return replyMessage;
		
		
	}
	
	void decodeOptiContent(byte[] buff, ReplyMessageContent replyMessage) throws OSCARSServiceException{
		PrimitiveDecoder priDecoder = new PrimitiveDecoder();
		ReplyPathContent replyPath = null;
		ReplyCoSchedulePathContent coSchedulePath = null;
		List<ReplyPathContent> altPaths = null;
		int altPathsNumber = 0;
		byte type = 0;	
		int length = 0;
		
		coSchedulePath = new ReplyCoSchedulePathContent();
		replyMessage.setCoScheduleReply(coSchedulePath);  //set coSchedulePath to reply message
		
		replyPath = this.decodePath(buff);
		
		coSchedulePath.setPath(replyPath);
		
		altPaths = coSchedulePath.getAltPathContent();
		
		type = buff[offset++];
		if(type == CodeNumber.PCE_ALT_PATH_NUM){
			length = this.decodeLength(priDecoder, buff);
			altPathsNumber = priDecoder.decodeInteger(buff, offset, length);
			offset = offset + length;			
		}
		
		
		for(int i=0;i<altPathsNumber;i++){
			replyPath = this.decodePath(buff);
			altPaths.add(replyPath);
		}		
		
	}

	void decodeReplyPathContent(byte[] buff, int totalLength, ReplyMessageContent replyMessage) throws OSCARSServiceException{
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
		int regularReplyLength = 0;
		int optionalReplyLength = 0;
		ReplyPathContent replyPath = null;
		ReplyCoSchedulePathContent coSchedulePath = null;
		List<ReplyPathContent> altPaths = null;
		int altPathsNumber = 0;
		
		
		replyPath = this.decodePath(buff);
		replyMessage.setReplyPathContent(replyPath);
		/*
		initialDecodeOffset = this.offset;		
		
				
		type = buff[offset++];
		if(type == CodeNumber.PCE_GRI){
			lengthTagSize = priDecoder.getLengthTagSize(buff, offset);
			length = priDecoder.getLength(buff, offset);
			offset = offset + lengthTagSize;
			gri = priDecoder.decodeString(buff, offset, length);
						
			offset = offset + length;
			replyMessage.setGri(gri);
		}
		
		type = buff[offset++];
		if(type==CodeNumber.PCE_COMPUTE_ERROR){
			length = this.decodeLength(priDecoder, buff);
			errorMessage = priDecoder.decodeString(buff, offset, length);
			offset = offset + length;			
			replyMessage.setErrorMessage(errorMessage);
			return;
		}
		
		if(type==CodeNumber.PCE_REGU_REPLY){
			regularReplyLength = this.decodeLength(priDecoder, buff);
			replyPath = this.decodePath(buff);
			replyMessage.setReplyPathContent(replyPath);
		}else{
			
		}
		
		if((this.offset - initialDecodeOffset) < totalLength)
		{
			type = buff[offset++];
			if(type==CodeNumber.PCE_OPTI_REPLY){
				optionalReplyLength = this.decodeLength(priDecoder, buff);
				coSchedulePath = new ReplyCoSchedulePathContent();
				replyMessage.setCoScheduleReply(coSchedulePath);  //set coSchedulePath to reply message
				
				replyPath = this.decodePath(buff);
				
				coSchedulePath.setPath(replyPath);
				
				altPaths = coSchedulePath.getAltPathContent();
				
				type = buff[offset++];
				length = this.decodeLength(priDecoder, buff);
				altPathsNumber = priDecoder.decodeInteger(buff, offset, length);
				offset = offset + length;
				
				for(int i=0;i<altPathsNumber;i++){
					replyPath = this.decodePath(buff);
					altPaths.add(replyPath);
				}				
			}else{
				
			}
		}
		*/
		/*
		if(type == CodeNumber.PCE_PATH_ID){
			length = this.decodeLength(priDecoder, buff);
			pathId = priDecoder.decodeString(buff, offset, length);
						
			offset = offset + length;
			ReplyPathContent replyPath = new ReplyPathContent();
			replyMessage.setReplyPathContent(replyPath);  //set replyPathContent to object variable
			replyMessage.getReplyPathContent().setId(pathId);
			
			type = buff[offset++];			
			if(type == CodeNumber.PCE_PATH_LENGTH){
				length = this.decodeLength(priDecoder, buff);
				pathLength = priDecoder.decodeInteger(buff, offset, length);
								
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
										
					offset = offset + length;
					
					replyLink.setSwitchingType(switchingCapType);					
				}else if(type == CodeNumber.PCE_SWITCHINGENCTYPE){
					length = this.decodeLength(priDecoder, buff);
					switchingEncType = priDecoder.decodeString(buff, offset, length);
										
					offset = offset + length;
					
					replyLink.setEncodingType(switchingEncType);
				}else if(type == CodeNumber.PCE_SWITCHINGVLANRANGEAVAI){
					length = this.decodeLength(priDecoder, buff);
					availableVlanTags = priDecoder.decodeString(buff, offset, length);
										
					offset = offset + length;
					
					replyLink.setAvailableVlanTags(availableVlanTags);
				}else if(type == CodeNumber.PCE_SWITCHINGVLANRANGESUGG){
					length = this.decodeLength(priDecoder, buff);
					suggestedVlanTags = priDecoder.decodeString(buff, offset, length);
										
					offset = offset + length;
					
					replyLink.setSuggestedVlanTags(suggestedVlanTags);
				}else if(type == CodeNumber.PCE_SWITCHINGVLANRANGEASSI){
					length = this.decodeLength(priDecoder, buff);
					assignedVlanTags = priDecoder.decodeString(buff, offset, length);
										
					offset = offset + length;
					
					replyLink.setAssignedVlanTags(assignedVlanTags);
				}else if(type == CodeNumber.PCE_VLANTRANSLATION){
					length = this.decodeLength(priDecoder, buff);
					vlanTranslation = priDecoder.decodeBoolean(buff, offset, length);
										
					offset = offset + length;
					
					replyLink.setVlanTranslation(vlanTranslation);
				}else if(type == CodeNumber.PCE_CAPACITY){
					length = this.decodeLength(priDecoder, buff);
					capacity = priDecoder.decodeInteger(buff, offset, length);
										
					offset = offset + length;
					
					replyLink.setCapacity(capacity);
				}else if(type == CodeNumber.PCE_MTU){
					length = this.decodeLength(priDecoder, buff);
					mtu = priDecoder.decodeInteger(buff, offset, length);
										
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
		*/
		
	}
	
	
	ReplyPathContent decodePath(byte[] buff) throws OSCARSServiceException{		
		PrimitiveDecoder priDecoder = new PrimitiveDecoder();
		byte type = 0;
		int lengthTagSize = 0;
		int length = 0;
		String gri;
		String errorMessage;
		String pathId = null;
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
		boolean pathEndFlag = false;
		List<ReplyBagSegmentContent> bags = null;
		ReplyBagSegmentContent bagSegment = null;
		int bagBandwidth;
		int bagStartTime;
		int bagEndTime;
		
		
		ReplyPathContent replyPath = new ReplyPathContent();
		
		type = buff[offset++];
		if(type == CodeNumber.PCE_PATH_ID){
			length = this.decodeLength(priDecoder, buff);
			pathId = priDecoder.decodeString(buff, offset, length);

			offset = offset + length;
			
			pathEndFlag = true;
		}
		
		replyPath.setId(pathId);
		

		type = buff[offset++];			
		if(type == CodeNumber.PCE_PATH_LENGTH){
			length = this.decodeLength(priDecoder, buff);
			pathLength = priDecoder.decodeInteger(buff, offset, length);

			offset = offset + length;

		}

		linkSet = replyPath.getReplyLinkContent();
		bags = replyPath.getReplyBagSegmentContent();

		while(pathEndFlag){
			type = buff[offset++];
			if(type == CodeNumber.PCE_LINK_ID){
				if(replyLink != null){
					linkSet.add(replyLink);
				}
				replyLink = new ReplyLinkContent();
				length = this.decodeLength(priDecoder, buff);
				linkName = priDecoder.decodeString(buff, offset, length);

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

				offset = offset + length;

				replyLink.setSwitchingType(switchingCapType);					
			}else if(type == CodeNumber.PCE_SWITCHINGENCTYPE){
				length = this.decodeLength(priDecoder, buff);
				switchingEncType = priDecoder.decodeString(buff, offset, length);

				offset = offset + length;

				replyLink.setEncodingType(switchingEncType);
			}else if(type == CodeNumber.PCE_SWITCHINGVLANRANGEAVAI){
				length = this.decodeLength(priDecoder, buff);
				availableVlanTags = priDecoder.decodeString(buff, offset, length);

				offset = offset + length;

				replyLink.setAvailableVlanTags(availableVlanTags);
			}else if(type == CodeNumber.PCE_SWITCHINGVLANRANGESUGG){
				length = this.decodeLength(priDecoder, buff);
				suggestedVlanTags = priDecoder.decodeString(buff, offset, length);

				offset = offset + length;

				replyLink.setSuggestedVlanTags(suggestedVlanTags);
			}else if(type == CodeNumber.PCE_SWITCHINGVLANRANGEASSI){
				length = this.decodeLength(priDecoder, buff);
				assignedVlanTags = priDecoder.decodeString(buff, offset, length);

				offset = offset + length;

				replyLink.setAssignedVlanTags(assignedVlanTags);
			}else if(type == CodeNumber.PCE_VLANTRANSLATION){
				length = this.decodeLength(priDecoder, buff);
				vlanTranslation = priDecoder.decodeBoolean(buff, offset, length);

				offset = offset + length;

				replyLink.setVlanTranslation(vlanTranslation);
			}else if(type == CodeNumber.PCE_CAPACITY){
				length = this.decodeLength(priDecoder, buff);
				capacity = priDecoder.decodeInteger(buff, offset, length);

				offset = offset + length;

				replyLink.setCapacity(capacity);
			}else if(type == CodeNumber.PCE_MTU){
				length = this.decodeLength(priDecoder, buff);
				mtu = priDecoder.decodeInteger(buff, offset, length);

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
			}else if(type == CodeNumber.PCE_OPT_BAG_BANDWIDTH){				
				length = this.decodeLength(priDecoder, buff);
				bagBandwidth = priDecoder.decodeInteger(buff, offset, length);
				
				offset = offset + length;
				
				if(bagSegment == null){
					bagSegment = new ReplyBagSegmentContent();
				}
				
				bagSegment.setBandwidth(bagBandwidth);					
			}else if(type == CodeNumber.PCE_OPT_BAG_STARTTIME){
				length = this.decodeLength(priDecoder, buff);
				bagStartTime = priDecoder.decodeInteger(buff, offset, length);
				
				offset = offset + length;
				
				if(bagSegment == null){
					throw new OSCARSServiceException("BagSegment should start with bandwidth in API encoded message");
				}
				bagSegment.setStartTime(bagStartTime);					
			}else if(type == CodeNumber.PCE_OPT_BAG_ENDTIME){
				length = this.decodeLength(priDecoder, buff);
				bagEndTime = priDecoder.decodeInteger(buff, offset, length);
				
				offset = offset + length;
				
				if(bagSegment == null){
					throw new OSCARSServiceException("BagSegment should start with bandwidth in API encoded message");
				}				
				bagSegment.setEndTime(bagEndTime);
				bags.add(bagSegment);
			}else if(type == CodeNumber.PCE_PATH_END_TAG){
				length = this.decodeLength(priDecoder, buff);
				String tempStr = priDecoder.decodeString(buff, offset, length);
				
				offset = offset + length;
				
				linkSet.add(replyLink); //last link	
				pathEndFlag = false;

			}else{
				
			}

		}
		//linkSet.add(replyLink); //last link			

		return replyPath;

		
		
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
