package net.es.oscars.pce.tce.optionalConstraint.stornet;

import javax.xml.transform.*;
import javax.xml.transform.stream.*;
import javax.xml.transform.dom.*;
import org.w3c.dom.*;

import javax.xml.parsers.*;
import java.io.*;
import java.util.List;

public class BuildXml {
	
	public void generateXml(CoScheduleReplyField coScheduleReply){
		
		try{
			DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
			DocumentBuilder builder = factory.newDocumentBuilder();
			Document document = builder.newDocument();
			generateDocument(document, coScheduleReply);
			
			
			TransformerFactory transFactory = TransformerFactory.newInstance();
			Transformer transformer = transFactory.newTransformer();
			DOMSource domSource = new DOMSource(document);
			File file = new File("/home/dell/test_out.xml");
			FileOutputStream out = new FileOutputStream(file);
			//StringWriter outStr = new StringWriter();
			StreamResult xmlResult = new StreamResult(out);
			//StreamResult xmlResult = new StreamResult(outStr);
			transformer.transform(domSource, xmlResult);
			
			//outStr.close();
			//String result = outStr.toString();
			//System.out.println(result);
		}catch(Exception e){
			System.out.println(e);
		}
	}
	
	public String generateXmlString(CoScheduleReplyField coScheduleReply){
		String result = null;
		
		try{
			DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
			DocumentBuilder builder = factory.newDocumentBuilder();
			Document document = builder.newDocument();
			generateDocument(document, coScheduleReply);
			
			
			TransformerFactory transFactory = TransformerFactory.newInstance();
			Transformer transformer = transFactory.newTransformer();
			DOMSource domSource = new DOMSource(document);
			//File file = new File("/home/dell/test_out.xml");
			//FileOutputStream out = new FileOutputStream(file);
			StringWriter outStr = new StringWriter();
			//StreamResult xmlResult = new StreamResult(out);
			StreamResult xmlResult = new StreamResult(outStr);
			transformer.transform(domSource, xmlResult);
			
			outStr.close();
			result = outStr.toString();
			//System.out.println("xml string="+result);
			
		}catch(Exception e){
			System.out.println(e);
		}
		return result;
	}
	
	protected void generateDocument(Document document, CoScheduleReplyField coScheduleReply){
		document.setXmlVersion("1.0");
		Element optionalConstraintNode = document.createElement("optionalConstraint");
		optionalConstraintNode.setAttribute("category", "api-experiment-stornet");
		document.appendChild(optionalConstraintNode);
		Element valueNode = document.createElement("value");
		optionalConstraintNode.appendChild(valueNode);
		Element coScheduleReplyNode = document.createElement("coScheduleReply");
		coScheduleReplyNode.setAttribute("id", coScheduleReply.getId());
		valueNode.appendChild(coScheduleReplyNode);
		List<CoSchedulePathField> coSchedulePath = coScheduleReply.getCoSchedulePath();
		int pathSize = coSchedulePath.size();
		for(int i=0;i<pathSize;i++){
			CoSchedulePathField coSchedulePathField = coSchedulePath.get(i);
			Element coSchedulePathNode = document.createElement("coSchedulePath");
			coSchedulePathNode.setAttribute("id", coSchedulePathField.getId());
			coScheduleReplyNode.appendChild(coSchedulePathNode);
			Element pathInfoNode = document.createElement("pathInfo");
			coSchedulePathNode.appendChild(pathInfoNode);			
			PathInfoField pathInfo = coSchedulePathField.getPathInfoField();
			Element pathNode = document.createElement("path");
			pathNode.setAttribute("id", pathInfo.getPathId());
			pathInfoNode.appendChild(pathNode);
						
			List<HopField> hop = pathInfo.getHop();
			int hopSize = hop.size();
			for(int j=0;j<hopSize;j++){
				HopField hopField = hop.get(j);
				Element hopNode = document.createElement("hop");
				hopNode.setAttribute("id", hopField.getHopId());
				pathNode.appendChild(hopNode);
				LinkField link = hopField.getLink();
				Element linkNode = document.createElement("link");
				linkNode.setAttribute("id", link.getLinkId());
				hopNode.appendChild(linkNode);
				Element SwitchingCapabilityDescriptorsNode = document.createElement("SwitchingCapabilityDescriptors");
				linkNode.appendChild(SwitchingCapabilityDescriptorsNode);
				
				Element switchingcapTypeNode = document.createElement("switchingcapType");
				Text switchingcapTypeValue = document.createTextNode(link.getSwitchingcapType());
				switchingcapTypeNode.appendChild(switchingcapTypeValue);
				
				SwitchingCapabilityDescriptorsNode.appendChild(switchingcapTypeNode);
				
				Element encodingTypeNode = document.createElement("encodingType");
				Text encodingTypeValue = document.createTextNode(link.getEncodingType());
				encodingTypeNode.appendChild(encodingTypeValue);
				
				SwitchingCapabilityDescriptorsNode.appendChild(encodingTypeNode);
				
				Element switchingCapabilitySpecificInfoNode = document.createElement("switchingCapabilitySpecificInfo");
				SwitchingCapabilityDescriptorsNode.appendChild(switchingCapabilitySpecificInfoNode);
				
				Element vlanRangeAvailabilityNode = document.createElement("vlanRangeAvailability");
				Text vlanRangeAvailabilityValue = document.createTextNode(link.getVlanRangeAvailability());
				vlanRangeAvailabilityNode.appendChild(vlanRangeAvailabilityValue);
				
				switchingCapabilitySpecificInfoNode.appendChild(vlanRangeAvailabilityNode);				
			}
			
			Element bagInfoNode = document.createElement("bagInfo");
			coSchedulePathNode.appendChild(bagInfoNode);
			BagInfoField bagInfo = coSchedulePathField.getBagInfoField();
			List<BagSegmentField> bagSegment = bagInfo.getBagSegment();
			int bagSize = bagSegment.size();
			for(int k=0;k<bagSize;k++){
				BagSegmentField bagSegmentField = bagSegment.get(k);
				Element bagSegmentNode = document.createElement("bagSegment");
				bagInfoNode.appendChild(bagSegmentNode);
				
				Element segmentBandwidthNode = document.createElement("segmentBandwidth");
				Text segmentBandwidthValue = document.createTextNode(Integer.toString(bagSegmentField.getSegmentBandwidth()));
				segmentBandwidthNode.appendChild(segmentBandwidthValue);
				bagSegmentNode.appendChild(segmentBandwidthNode);
				
				Element segmentStartTimeNode = document.createElement("segmentStartTime");
				Text segmentStartTimeValue = document.createTextNode(Integer.toString(bagSegmentField.getSegmentStartTime()));
				segmentStartTimeNode.appendChild(segmentStartTimeValue);
				bagSegmentNode.appendChild(segmentStartTimeNode);
				
				Element segmentEndTimeNode = document.createElement("segmentEndTime");
				Text segmentEndTimeValue = document.createTextNode(Integer.toString(bagSegmentField.getSegmentEndTime()));
				segmentEndTimeNode.appendChild(segmentEndTimeValue);
				bagSegmentNode.appendChild(segmentEndTimeNode);
			}
			
		}
	}

}
