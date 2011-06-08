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
		//UserRequestConstraintType userConType = pceData.getUserRequestConstraint()
		//CtrlPlaneTopologyContent topology = pceData.getTopology();
		String replyGri = replyMessage.getGri();
		String errorMsg = replyMessage.getErrorMessage();
		if(gri.equals(replyGri)){
			//System.out.println("test-1-1");
			if(errorMsg==null){
				//System.out.println("test-1-2");
				this.writeResvConstraint(replyMessage, resConType);
				this.writeTopology(replyMessage, pceData);
			}else{
				//System.out.println("Error message is: " + errorMsg);
				throw new OSCARSServiceException("Path computation fails with message: " + errorMsg);
			}
		}else{
			//System.out.println("Error:Gri is not same.");
			throw new OSCARSServiceException("Gri returned by API message is not as same as request");
		}
		
	}
	
	protected void writeResvConstraint(ReplyMessageContent replyMessage, ReservedConstraintType resConType){
		ReplyPathContent path = replyMessage.getReplyPathContent();
		List<ReplyLinkContent> links = path.getReplyLinkContent();
		List<CtrlPlaneHopContent> hop = resConType.getPathInfo().getPath().getHop();
		
		int linkSize = links.size();
		
		/*
		if(hopSize == linkSize){
			for(int i=0;i<hopSize;i++){
				CtrlPlaneLinkContent linkOri = hop.get(i).getLink();
				ReplyLinkContent linkRes = links.get(i);
				if(linkOri.getId().equals(linkRes.getId())){
					
					
				}else{
					System.out.println("Error:Link is not same");
				}
				
				
			}
			
		}else{
			System.out.println("Error:Hop count is not same.");
		}
		*/
		//System.out.println("test-1-0 hopsize=" + hopSize);
		while(hop.size() !=0){
			//System.out.println("test-1-0 hopsize=" + hop.size());
			hop.remove(0);
		}
		//System.out.println("test-1-3");
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
			//switchingCapabilitySpecificInfo.setCapability(Integer.toString(capability));
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
		//System.out.println("test-1-4");
	}
	
	protected void writeTopology(ReplyMessageContent replyMessage, PCEDataContent pceData) throws OSCARSServiceException{
		ReplyPathContent path = replyMessage.getReplyPathContent();
		List<ReplyLinkContent> links = path.getReplyLinkContent();
		//List<CtrlPlaneHopContent> hop = null;
		NMWGTopoBuilder topoBuilder = new NMWGTopoBuilder();
		
		
		int linkSize = links.size();
		
		/*
		if(hopSize == linkSize){
			for(int i=0;i<hopSize;i++){
				CtrlPlaneLinkContent linkOri = hop.get(i).getLink();
				ReplyLinkContent linkRes = links.get(i);
				if(linkOri.getId().equals(linkRes.getId())){
					
					
				}else{
					System.out.println("Error:Link is not same");
				}
				
				
			}
			
		}else{
			System.out.println("Error:Hop count is not same.");
		}
		*/
		//System.out.println("test-1-0 hopsize=" + hopSize);
		//while(hop.size() !=0){
			//System.out.println("test-1-0 hopsize=" + hop.size());
			//hop.remove(0);
		//}
		//System.out.println("test-1-3");
		for(int i=0;i<linkSize;i++){
			//CtrlPlaneHopContent hopContent = new CtrlPlaneHopContent();
			CtrlPlaneLinkContent linkOri = new CtrlPlaneLinkContent();
			CtrlPlaneSwcapContent switchingCapabilityDescriptors = new CtrlPlaneSwcapContent();
			CtrlPlaneSwitchingCapabilitySpecificInfo switchingCapabilitySpecificInfo = new CtrlPlaneSwitchingCapabilitySpecificInfo();
			ReplyLinkContent linkRes = links.get(i);
			//String hopId = UUID.randomUUID().toString();
			//hopContent.setId(hopId);
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
			//switchingCapabilitySpecificInfo.setCapability(Integer.toString(capability));
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
		//System.out.println("test-1-4");
		pceData.setTopology(topoBuilder.getTopology());
		//System.out.println("test-1-4");
	}

}
