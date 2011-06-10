package net.es.oscars.pce.tce;

import java.util.ArrayList;
import java.util.List;
import javax.xml.bind.annotation.XmlAccessType;
import javax.xml.bind.annotation.XmlAccessorType;
import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlType;
import net.es.oscars.api.soap.gen.v06.*;
import net.es.oscars.utils.soap.OSCARSServiceException;

import org.ogf.schema.network.topology.ctrlplane.*;



public class EncodePceMessage {
	
    public static final byte PCE_USERCONSTRAINT = (byte)0xF1;
    public static final byte PCE_RESVCONSTRAINT = (byte)0xF2;
	
	public static final byte PCE_GRI = (byte)0x81;
	public static final byte PCE_LOGIN = (byte)0x82;
	public static final byte PCE_LAYER = (byte)0x83;
	public static final byte PCE_SOURCE = (byte)0x84;
	public static final byte PCE_DESTINATION = (byte)0x85;
	public static final byte PCE_BANDWIDTH = (byte)0x86;
	public static final byte PCE_DESCRIPTION = (byte)0x87;
	public static final byte PCE_STARTTIME = (byte)0x88;
	public static final byte PCE_ENDTIME = (byte)0x89;
	public static final byte PCE_PATHSETUPMODEL = (byte)0x8A;
	public static final byte PCE_PATHTYPE = (byte)0x8B;
	public static final byte PCE_SRCVLAN = (byte)0x8C;
	public static final byte PCE_DESTVLAN = (byte)0x8D;
	public static final byte PCE_PATH = (byte)0x8E;
	public static final byte PCE_SRCIPPORT = (byte)0x8F;
	public static final byte PCE_DESTIPPORT = (byte)0x90;
	public static final byte PCE_L3_PROTOCOL = (byte)0x91;
	public static final byte PCE_L3_DSCP = (byte)0x92;
	public static final byte PCE_MPLS_BURSTLIMIT = (byte)0x93;
	public static final byte PCE_MPLS_LSPCLASS = (byte)0x94;

	
	public static final byte PCE_PATH_ID = (byte)0x95;
	public static final byte PCE_HOP_ID = (byte)0x96;
	public static final byte PCE_LINK_ID = (byte)0x97;
	public static final byte PCE_SWITCHINGCAPTYPE = (byte)0x98;
	public static final byte PCE_SWITCHINGENCTYPE = (byte)0x99;
	public static final byte PCE_SWITCHINGVLANRANGEAVAI = (byte)0x9A;
	public static final byte PCE_PATH_LENGTH = (byte)0x9B;
	public static final byte PCE_SWITCHINGVLANRANGESUGG = (byte)0x9C;
	public static final byte PCE_VLANTRANSLATION = (byte)0x9D;
	public static final byte PCE_SWITCHINGVLANRANGEASSI = (byte)0x9E;
	public static final byte PCE_CAPACITY = (byte)0x9F;
	public static final byte PCE_MTU = (byte)0xA0;

	
	public static final byte ASN_LONG_LEN = (byte)0x80;
	
	
	byte[] pceEncodeArray;
		
	public EncodePceMessage(){
		this.pceEncodeArray = null;		
	}
	
	public void encodeUserConstraint (String gri, UserRequestConstraintType userCons)throws OSCARSServiceException
	{		
		byte[] encodeBuffHead;
		byte[] encodeBuffBody;
		byte[] encodeBuff;
		byte pceType = PCE_USERCONSTRAINT;
		PrimitiveEncoder priEncoder = new PrimitiveEncoder();
		int buffLen;
	    long startTime = userCons.getStartTime();
	    long endTime = userCons.getEndTime();
	    int bandwidth = (userCons.getBandwidth()) * 1000000; //convert from MB to Bytes
	    PathInfo pathInf = userCons.getPathInfo();
	    String pathSetupMode = pathInf.getPathSetupMode();
	    String pathType = pathInf.getPathType();
	    
	    if((pathInf.getLayer2Info()!=null) && (pathInf.getLayer3Info()!=null)){
	    	throw new OSCARSServiceException("The request has both layer2 and layer3 information");
	    }
	    
	    priEncoder.encodeString(PCE_GRI, gri);
	    priEncoder.encodeLong(PCE_STARTTIME, startTime);
	    priEncoder.encodeLong(PCE_ENDTIME, endTime);
	    priEncoder.encodeInteger(PCE_BANDWIDTH, bandwidth);	    
	    
	    if(pathInf.getLayer2Info()!=null){
	    	Layer2Info layer2Info = pathInf.getLayer2Info();
	    	VlanTag srcVtag = layer2Info.getSrcVtag();
	    	VlanTag destVtag = layer2Info.getDestVtag();
	    	String srcEndPoint = layer2Info.getSrcEndpoint();
	    	String destEndPoint = layer2Info.getDestEndpoint();
	    	String srcVtagValue = srcVtag.getValue();
	    	String destVtagValue = destVtag.getValue();

	    	priEncoder.encodeString(PCE_LAYER, "2");
	    	priEncoder.encodeString(PCE_SOURCE, srcEndPoint);
	    	priEncoder.encodeString(PCE_DESTINATION, destEndPoint);
	    	priEncoder.encodeString(PCE_SRCVLAN, srcVtagValue);
	    	priEncoder.encodeString(PCE_DESTVLAN, destVtagValue);
	    }	    
	    
	    if(pathInf.getLayer3Info() != null){
	    	Layer3Info layer3Info = pathInf.getLayer3Info();
	    	String srcHost = layer3Info.getSrcHost();
	    	String destHost = layer3Info.getDestHost();
	    	String protocol = layer3Info.getProtocol();
	    	String dscp = layer3Info.getDscp();
	    	int srcIpPort = layer3Info.getSrcIpPort();
	    	int destIpPort = layer3Info.getDestIpPort();
	    	
	    	priEncoder.encodeString(PCE_LAYER, "3");
	    	priEncoder.encodeString(PCE_SOURCE, srcHost);
	    	priEncoder.encodeString(PCE_DESTINATION, destHost);
	    	priEncoder.encodeInteger(PCE_SRCIPPORT, srcIpPort);
	    	priEncoder.encodeInteger(PCE_DESTIPPORT, destIpPort);
	    	priEncoder.encodeString(PCE_L3_PROTOCOL, protocol);
	    	priEncoder.encodeString(PCE_L3_DSCP, dscp);	    	
	    }
	    
	    priEncoder.encodeString(PCE_PATHSETUPMODEL, pathSetupMode);
	    priEncoder.encodeString(PCE_PATHTYPE, pathType);
	    
	    if(pathInf.getMplsInfo() != null){
	    	MplsInfo mplsInfo = pathInf.getMplsInfo();
	    	int burstLimit = mplsInfo.getBurstLimit();
	    	String lspClass = mplsInfo.getLspClass();
	    	
	    	priEncoder.encodeInteger(PCE_MPLS_BURSTLIMIT, burstLimit);
	    	priEncoder.encodeString(PCE_MPLS_LSPCLASS, lspClass);
	    }
	    
	    priEncoder.buffPrune();
	    encodeBuffBody = priEncoder.getBuff();
	    buffLen = encodeBuffBody.length;
	    
	    encodeBuffHead = this.encodePceHeader(pceType, buffLen);
	    encodeBuff = this.mergeBuff(encodeBuffHead, encodeBuffBody);
	    
	    this.combineField(encodeBuff);	    
	    	   
	}
	
	public void encodeResvConstraint (ReservedConstraintType resvCons)throws OSCARSServiceException
	{		
		byte[] encodeBuffHead;
		byte[] encodeBuffBody;
		byte[] encodeBuff;
		byte pceType = PCE_RESVCONSTRAINT;
		PrimitiveEncoder priEncoder = new PrimitiveEncoder();
		int buffLen;
	    long startTime = resvCons.getStartTime();
	    long endTime = resvCons.getEndTime();
	    int bandwidth = resvCons.getBandwidth();
	    PathInfo pathInf = resvCons.getPathInfo();
	    
	    if((pathInf.getLayer2Info()!=null) && (pathInf.getLayer3Info()!=null)){
	    	throw new OSCARSServiceException("The request has both layer2 and layer3 information");
	    }

	    priEncoder.encodeLong(PCE_STARTTIME, startTime);
	    priEncoder.encodeLong(PCE_ENDTIME, endTime);
	    priEncoder.encodeInteger(PCE_BANDWIDTH, bandwidth);	
	    
	    if(pathInf!=null){
		    if(pathInf.getPathSetupMode()!=null){
		    	String pathSetupMode = pathInf.getPathSetupMode();
		    	priEncoder.encodeString(PCE_PATHSETUPMODEL, pathSetupMode);
		    }

		    if(pathInf.getPathType()!=null){
		    	String pathType = pathInf.getPathType();
		    	priEncoder.encodeString(PCE_PATHTYPE, pathType);
		    }
		    
		   
		    if(pathInf.getLayer2Info()!=null){
		    	
		    	Layer2Info layer2Info = pathInf.getLayer2Info();
		    	
		    	VlanTag srcVtag = layer2Info.getSrcVtag();
		    	VlanTag destVtag = layer2Info.getDestVtag();
		    	String srcEndPoint = layer2Info.getSrcEndpoint();
		    	String destEndPoint = layer2Info.getDestEndpoint();
		    	String srcVtagValue = srcVtag.getValue();
		    	String destVtagValue = destVtag.getValue();
		    	
		    	priEncoder.encodeString(PCE_LAYER, "2");
		    	priEncoder.encodeString(PCE_SOURCE, srcEndPoint);
		    	priEncoder.encodeString(PCE_DESTINATION, destEndPoint);
		    	priEncoder.encodeString(PCE_SRCVLAN, srcVtagValue);
		    	priEncoder.encodeString(PCE_DESTVLAN, destVtagValue);
		    }
		   
		    
		    if(pathInf.getLayer3Info() != null){

		    	Layer3Info layer3Info = pathInf.getLayer3Info();
		    	String srcHost = layer3Info.getSrcHost();
		    	String destHost = layer3Info.getDestHost();
		    	String protocol = layer3Info.getProtocol();
		    	String dscp = layer3Info.getDscp();
		    	int srcIpPort = layer3Info.getSrcIpPort();
		    	int destIpPort = layer3Info.getDestIpPort();
		    	
		    	priEncoder.encodeString(PCE_LAYER, "3");
		    	priEncoder.encodeString(PCE_SOURCE, srcHost);
		    	priEncoder.encodeString(PCE_DESTINATION, destHost);
		    	priEncoder.encodeInteger(PCE_SRCIPPORT, srcIpPort);
		    	priEncoder.encodeInteger(PCE_DESTIPPORT, destIpPort);
		    	priEncoder.encodeString(PCE_L3_PROTOCOL, protocol);
		    	priEncoder.encodeString(PCE_L3_DSCP, dscp);	    	
		    }
		    

		    if(pathInf.getMplsInfo() != null){

		    	MplsInfo mplsInfo = pathInf.getMplsInfo();
		    	int burstLimit = mplsInfo.getBurstLimit();
		    	String lspClass = mplsInfo.getLspClass();
		    	
		    	priEncoder.encodeInteger(PCE_MPLS_BURSTLIMIT, burstLimit);
		    	priEncoder.encodeString(PCE_MPLS_LSPCLASS, lspClass);
		    }
	    	
	    	if(pathInf.getPath()!=null){

	    		CtrlPlanePathContent path = pathInf.getPath();
	    		if(path.getId()!=null){
	    			String pathId = path.getId();

	    			priEncoder.encodeString(PCE_PATH_ID, pathId);
	    		}

	    		
	    		if(path.getHop()!=null){
	    			List<CtrlPlaneHopContent> hop = path.getHop();
	    			priEncoder.encodeInteger(PCE_PATH_LENGTH, hop.size());
	    			CtrlPlaneHopContent oneHop;
	    			for(int i=0;i<hop.size();i++){
	    				 oneHop = hop.get(i);
	    				 if(oneHop.getId()!=null){
	    					 String hopId = oneHop.getId();

	    					 priEncoder.encodeString(PCE_HOP_ID, hopId);
	    				 }
    					 

	    				 if(oneHop.getLink()!=null){
	    					 CtrlPlaneLinkContent link = oneHop.getLink();
	    					 if(link.getId()!=null){
	    						 String linkId = link.getId();
	    						 
	    						 priEncoder.encodeString(PCE_LINK_ID, linkId);
	    					 }
	    					
	    					 if(link.getSwitchingCapabilityDescriptors()!=null){
	    						 CtrlPlaneSwcapContent switchingCapabilityDescriptors = link.getSwitchingCapabilityDescriptors();
	    						 if(switchingCapabilityDescriptors.getSwitchingcapType()!=null){
	    							 String switchingcapType = switchingCapabilityDescriptors.getSwitchingcapType();
	    							 
	    							 priEncoder.encodeString(PCE_SWITCHINGCAPTYPE, switchingcapType);
	    						 }
	    						 if(switchingCapabilityDescriptors.getEncodingType()!=null){
	    							 String encodingType = switchingCapabilityDescriptors.getEncodingType();
	    							 
	    							 priEncoder.encodeString(PCE_SWITCHINGENCTYPE, encodingType);
	    						 }
	    						 if(switchingCapabilityDescriptors.getSwitchingCapabilitySpecificInfo()!=null){
	    							 CtrlPlaneSwitchingCapabilitySpecificInfo switchingCapabilitySpecificInfo = switchingCapabilityDescriptors.getSwitchingCapabilitySpecificInfo();
	    							 
	    							 if(switchingCapabilitySpecificInfo.getVlanRangeAvailability()!=null){
	    								 String vlanRangeAvailability = switchingCapabilitySpecificInfo.getVlanRangeAvailability();
	    								 
	    								 priEncoder.encodeString(PCE_SWITCHINGVLANRANGEAVAI, vlanRangeAvailability);
	    							 }
	    							 
	    							 if(switchingCapabilitySpecificInfo.getSuggestedVLANRange()!=null){
	    								 String suggestedVLANRange = switchingCapabilitySpecificInfo.getSuggestedVLANRange();
	    								 priEncoder.encodeString(PCE_SWITCHINGVLANRANGESUGG, suggestedVLANRange);
	    							 }
	    							 
	    							 
	    							 if(switchingCapabilitySpecificInfo.isVlanTranslation()!=null){
		    							 if(switchingCapabilitySpecificInfo.isVlanTranslation()==true){
		    								 priEncoder.encodeBoolean(PCE_VLANTRANSLATION, true);
		    							 }
		    							 else{
		    								 priEncoder.encodeBoolean(PCE_VLANTRANSLATION, false);	    								 
		    							 }	    								 
	    								 
	    							 }

	    						 }//end if switchingCapabilityDescriptors
	    					 }//end if link.getSwitchingCapabilityDescriptors
	    				 }//end if oneHop.getLink
	    			}//end for int i=0;
	    		}//end if path.getHop
	    		
	    	}//end if pathInf.getPath
	    	
	    }
	    
	    priEncoder.buffPrune();
	    encodeBuffBody = priEncoder.getBuff();
	    buffLen = encodeBuffBody.length;
	    
	    encodeBuffHead = this.encodePceHeader(pceType, buffLen);
	    encodeBuff = this.mergeBuff(encodeBuffHead, encodeBuffBody);
	    
	    this.combineField(encodeBuff);	    
	    	   
	}	
	
	public byte[] encodePceHeader(byte pceType, int length){
		int size = 0;
		byte[] pceMessHeadBuff;
		int offset = 0;
		
		if(length<0x80){
			size = 1;
		}
		else if(length<=0xFF){
			size = 2;
		}
		else if(length<=0xFFFF){
			size = 3;
		}
		else if(length<=0xFFFFFF){
			size = 4;
		}
		else{
			
		}
		size = size + 1;
		pceMessHeadBuff = new byte[size];
		
		pceMessHeadBuff[offset++] = pceType;
		//pceMessHeadBuff[offset++] = priType;
		
		if(length<0x80){
			pceMessHeadBuff[offset++] = (byte) (length & 0xFF);			
		}
		else if(length<=0xFF){
			pceMessHeadBuff[offset++] = (0x01 | ASN_LONG_LEN);
			pceMessHeadBuff[offset++] = (byte) (length & 0xFF);			
		}
		else if (length<=0xFFFF){
			pceMessHeadBuff[offset++] = (0x02 | ASN_LONG_LEN);
			pceMessHeadBuff[offset++] = (byte) ((length>>8) & 0xFF);
			pceMessHeadBuff[offset++] = (byte) (length & 0xFF);
		}
		else if (length<=0xFFFFFF){
			pceMessHeadBuff[offset++] = (0x03 | ASN_LONG_LEN);
			pceMessHeadBuff[offset++] = (byte) ((length>>16) & 0xFF);
			pceMessHeadBuff[offset++] = (byte) ((length>>8) & 0xFF);
			pceMessHeadBuff[offset++] = (byte) (length & 0xFF);
		}
		else{
			
		}
		
		return pceMessHeadBuff;
		
	}
	
	public byte[] mergeBuff(byte[] buff1, byte[] buff2){
		int length1 = buff1.length;
		int length2 = buff2.length;
		int lengthMerge = length1 + length2;
		byte[] mergeArray = new byte[lengthMerge];
		System.arraycopy(buff1, 0, mergeArray, 0, length1);
		System.arraycopy(buff2, 0, mergeArray, length1, length2);
		
		return mergeArray;
	}
	
	public void combineField(byte[] field){
		byte[] combArray;
		if(pceEncodeArray == null){
			combArray = field;
		}
		else{
			combArray = this.mergeBuff(pceEncodeArray, field);
		}
		pceEncodeArray = combArray;
	}
	
	public byte[] getPceEncodeArray(){
		return this.pceEncodeArray;
	}

}
