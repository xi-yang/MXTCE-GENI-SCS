package net.es.oscars.pce.tce;

import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.HashMap;

import org.apache.log4j.Logger;
import org.ogf.schema.network.topology.ctrlplane.CtrlPlaneDomainContent;
import org.ogf.schema.network.topology.ctrlplane.CtrlPlaneHopContent;
import org.ogf.schema.network.topology.ctrlplane.CtrlPlaneLinkContent;
import org.ogf.schema.network.topology.ctrlplane.CtrlPlaneNodeContent;
import org.ogf.schema.network.topology.ctrlplane.CtrlPlanePathContent;
import org.ogf.schema.network.topology.ctrlplane.CtrlPlanePortContent;
import org.ogf.schema.network.topology.ctrlplane.CtrlPlaneTopologyContent;

import net.es.oscars.api.soap.gen.v06.ListReply;
import net.es.oscars.api.soap.gen.v06.ListRequest;
import net.es.oscars.api.soap.gen.v06.ResDetails;
import net.es.oscars.common.soap.gen.AuthConditionType;
import net.es.oscars.common.soap.gen.AuthConditions;
import net.es.oscars.logging.ErrSev;
import net.es.oscars.logging.OSCARSNetLogger;
import net.es.oscars.pce.PCEMessage;
import net.es.oscars.pce.soap.gen.v06.PCEDataContent;
import net.es.oscars.utils.clients.RMClient;
import net.es.oscars.utils.sharedConstants.StateEngineValues;
import net.es.oscars.utils.soap.OSCARSServiceException;
import net.es.oscars.utils.topology.NMWGParserUtil;

import java.util.ArrayList;
import java.util.List;
import javax.xml.bind.annotation.XmlAccessType;
import javax.xml.bind.annotation.XmlAccessorType;
import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlType;
import net.es.oscars.api.soap.gen.v06.OptionalConstraintType;
import net.es.oscars.api.soap.gen.v06.ReservedConstraintType;
import net.es.oscars.api.soap.gen.v06.UserRequestConstraintType;
import org.ogf.schema.network.topology.ctrlplane.CtrlPlaneTopologyContent;

import net.es.oscars.common.soap.gen.MessagePropertiesType;
import net.es.oscars.pce.soap.gen.v06.PCEDataContent;

public class CreateApiMsg {
	private PCEMessage query;
	private byte[] encodedApiMessage;
	
	public CreateApiMsg(PCEMessage query){
		this.query = query;
				
	}
	
	public byte[] getEncodedApiMsg(){
		return this.encodedApiMessage;
	}
	
	public void encodeApiMsg() throws OSCARSServiceException{
		PCEDataContent pceData = query.getPCEDataContent();
		byte[] apiMsg;
		byte[] apiMsgHeader;
		//byte[] userCons;
		//byte[] resvCons;
		byte[] requestCons;
		String gri = query.getGri();
		
		ReservedConstraintType resConType = pceData.getReservedConstraint();
		UserRequestConstraintType userConType = pceData.getUserRequestConstraint();
		
		EncodePceMessage encodePceMess = new EncodePceMessage();
		
		if(userConType==null){
			throw new OSCARSServiceException("UserConstraint are empty");
		}
		
		encodePceMess.encodeUserConstraint(gri, userConType);
		
		if(resConType!=null){
			//System.out.println("resCon y");
			encodePceMess.encodeResvConstraint(resConType);
			
		}
	
		
		
		requestCons = encodePceMess.getPceEncodeArray();
		
		//System.out.println(requestCons.length);
		
		
		
		
		EncodeApiMsgHeader encoderApiMsgHeader = new EncodeApiMsgHeader();
		apiMsgHeader = encoderApiMsgHeader.encoderApiMsg((short)1, (short)requestCons.length, 6, 0, 0);
		apiMsg = encodePceMess.mergeBuff(apiMsgHeader, requestCons);
		this.encodedApiMessage = apiMsg;
	}

}
