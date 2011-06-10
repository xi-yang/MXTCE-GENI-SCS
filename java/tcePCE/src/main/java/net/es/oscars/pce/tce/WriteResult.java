package net.es.oscars.pce.tce;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

import net.es.oscars.api.soap.gen.v06.*;
import net.es.oscars.pce.PCEMessage;
import net.es.oscars.pce.soap.gen.v06.PCEDataContent;
import net.es.oscars.utils.soap.OSCARSServiceException;
import net.es.oscars.utils.topology.NMWGTopoBuilder;

import org.ogf.schema.network.topology.ctrlplane.*;

public class WriteResult {
	
	private PCEMessage query;
	
	public PCEMessage getQuery(){
		return this.query;
	}
	
	public WriteResult(PCEMessage query){
		this.query = query;
		
	}
	
	public void writeComputeRes(ReplyMessageContent replyMessage) throws OSCARSServiceException{
		PCEDataContent pceData = query.getPCEDataContent();
		String gri = query.getGri();		
		ReservedConstraintType resConType = pceData.getReservedConstraint();

		String replyGri = replyMessage.getGri();
		String errorMsg = replyMessage.getErrorMessage();
		if(gri.equals(replyGri)){
			
			if(errorMsg==null){
				
				this.writeResvConstraint(replyMessage, resConType);
				this.writeTopology(replyMessage, pceData);
			}else{
				
				throw new OSCARSServiceException("Path computation fails with message: " + errorMsg);
			}
		}else{
			
			throw new OSCARSServiceException("Gri returned by API message is not as same as request");
		}
		
	}
	
	protected void writeResvConstraint(ReplyMessageContent replyMessage, ReservedConstraintType resConType){
		ReplyPathContent path = replyMessage.getReplyPathContent();
		List<ReplyLinkContent> links = path.getReplyLinkContent();
		List<CtrlPlaneHopContent> hop = resConType.getPathInfo().getPath().getHop();
		
		int linkSize = links.size();
		

		while(hop.size() !=0){
			
			hop.remove(0);
		}
		
		for(int i=0;i<linkSize;i++){
			CtrlPlaneHopContent hopContent = new CtrlPlaneHopContent();
			CtrlPlaneLinkContent linkOri = new CtrlPlaneLinkContent();
			CtrlPlaneSwcapContent switchingCapabilityDescriptors = new CtrlPlaneSwcapContent();
			CtrlPlaneSwitchingCapabilitySpecificInfo switchingCapabilitySpecificInfo = new CtrlPlaneSwitchingCapabilitySpecificInfo();
			ReplyLinkContent linkRes = links.get(i);
			String hopId = UUID.randomUUID().toString();
			hopContent.setId(hopId);
			String linkId = linkRes.getName();
			linkOri.setId(linkId);
			
			String remoteLinkId = linkRes.getRemoteLinkId();
			if(remoteLinkId != null){
				linkOri.setRemoteLinkId(remoteLinkId);
			}else{
				linkOri.setRemoteLinkId("urn:ogf:network:domain=*:node=*:port=*:link=*");
			}
			int maximumReservableCapacity = linkRes.getMaximumReservableCapacity();
			linkOri.setMaximumReservableCapacity(Integer.toString(maximumReservableCapacity));
			int minimumReservableCapacity = linkRes.getMinimumReservableCapacity();
			linkOri.setMinimumReservableCapacity(Integer.toString(minimumReservableCapacity));
			int granularity = linkRes.getGranularity();
			linkOri.setGranularity(Integer.toString(granularity));
			int trafficEngineeringMetric = linkRes.getTrafficEngineeringMetric();
			linkOri.setTrafficEngineeringMetric(Integer.toString(trafficEngineeringMetric));
			String switchingcapType = linkRes.getSwitchingType();
			switchingCapabilityDescriptors.setSwitchingcapType(switchingcapType);
			String encodingType = linkRes.getEncodingType();
			switchingCapabilityDescriptors.setEncodingType(encodingType);
			int capability = linkRes.getCapacity();
			
			linkOri.setCapacity(Integer.toString(capability));
			int interfaceMTU = linkRes.getMtu();
			switchingCapabilitySpecificInfo.setInterfaceMTU(interfaceMTU);
			String vlanRangeAvailability = linkRes.getAvailableVlanTags();
			switchingCapabilitySpecificInfo.setVlanRangeAvailability(vlanRangeAvailability);
			String suggestedVLANRange = linkRes.getSuggestedVlanTags();
			switchingCapabilitySpecificInfo.setSuggestedVLANRange(suggestedVLANRange);
			boolean vlanTranslation = linkRes.getVlanTranslation();
			switchingCapabilitySpecificInfo.setVlanTranslation(vlanTranslation);
			
			switchingCapabilityDescriptors.setSwitchingCapabilitySpecificInfo(switchingCapabilitySpecificInfo);
			linkOri.setSwitchingCapabilityDescriptors(switchingCapabilityDescriptors);
			hopContent.setLink(linkOri);
			hop.add(hopContent);
		}
		
	}
	
	protected void writeTopology(ReplyMessageContent replyMessage, PCEDataContent pceData) throws OSCARSServiceException{
		ReplyPathContent path = replyMessage.getReplyPathContent();
		List<ReplyLinkContent> links = path.getReplyLinkContent();
		
		NMWGTopoBuilder topoBuilder = new NMWGTopoBuilder();
		
		
		int linkSize = links.size();
		

		for(int i=0;i<linkSize;i++){
			
			CtrlPlaneLinkContent linkOri = new CtrlPlaneLinkContent();
			CtrlPlaneSwcapContent switchingCapabilityDescriptors = new CtrlPlaneSwcapContent();
			CtrlPlaneSwitchingCapabilitySpecificInfo switchingCapabilitySpecificInfo = new CtrlPlaneSwitchingCapabilitySpecificInfo();
			ReplyLinkContent linkRes = links.get(i);
			
			
			String linkId = linkRes.getName();
			linkOri.setId(linkId);
			
			String remoteLinkId = linkRes.getRemoteLinkId();
			if(remoteLinkId != null){
				linkOri.setRemoteLinkId(remoteLinkId);
			}else{
				linkOri.setRemoteLinkId("urn:ogf:network:domain=*:node=*:port=*:link=*");
			}
			int maximumReservableCapacity = linkRes.getMaximumReservableCapacity();
			linkOri.setMaximumReservableCapacity(Integer.toString(maximumReservableCapacity));
			int minimumReservableCapacity = linkRes.getMinimumReservableCapacity();
			linkOri.setMinimumReservableCapacity(Integer.toString(minimumReservableCapacity));
			int granularity = linkRes.getGranularity();
			linkOri.setGranularity(Integer.toString(granularity));
			int trafficEngineeringMetric = linkRes.getTrafficEngineeringMetric();
			linkOri.setTrafficEngineeringMetric(Integer.toString(trafficEngineeringMetric));
			String switchingcapType = linkRes.getSwitchingType();
			switchingCapabilityDescriptors.setSwitchingcapType(switchingcapType);
			String encodingType = linkRes.getEncodingType();
			switchingCapabilityDescriptors.setEncodingType(encodingType);
			int capability = linkRes.getCapacity();
			
			linkOri.setCapacity(Integer.toString(capability));
			int interfaceMTU = linkRes.getMtu();
			switchingCapabilitySpecificInfo.setInterfaceMTU(interfaceMTU);
			String vlanRangeAvailability = linkRes.getAvailableVlanTags();
			switchingCapabilitySpecificInfo.setVlanRangeAvailability(vlanRangeAvailability);
			String suggestedVLANRange = linkRes.getSuggestedVlanTags();
			switchingCapabilitySpecificInfo.setSuggestedVLANRange(suggestedVLANRange);
			boolean vlanTranslation = linkRes.getVlanTranslation();
			switchingCapabilitySpecificInfo.setVlanTranslation(vlanTranslation);
			
			switchingCapabilityDescriptors.setSwitchingCapabilitySpecificInfo(switchingCapabilitySpecificInfo);
			linkOri.setSwitchingCapabilityDescriptors(switchingCapabilityDescriptors);
			topoBuilder.addLink(linkOri);

		}
		
		pceData.setTopology(topoBuilder.getTopology());
		
	}

}
