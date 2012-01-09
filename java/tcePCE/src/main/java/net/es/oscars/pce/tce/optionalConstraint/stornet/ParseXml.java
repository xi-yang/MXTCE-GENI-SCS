package net.es.oscars.pce.tce.optionalConstraint.stornet;

import java.io.*;
import java.util.*;
//import org.apache.log4j.*;
//import org.hibernate.*;

import javax.xml.parsers.*;
import javax.xml.transform.*;
import javax.xml.transform.stream.*;
import javax.xml.transform.dom.*;

import org.w3c.dom.*;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;

import java.util.ArrayList;
import java.util.List;

public class ParseXml {
	
	public CoScheduleRequestField readInput(Element inputName){
		
		String inputStr = inputName.getTextContent();
		
		//System.out.println(inputStr);
		return this.readInput(inputStr);
		
	}
	
	public CoScheduleRequestField readInput(String inputName){

		String optionalConsName = null;
		String coScheduleRequestId = null;
		String startTime = null;
		String endTime = null;
		String minBandwidth = null;
		String maxNumOfAltPaths = null;
		String bandwidthAvailabilityGraph = null; 
		String contiguousVlan = null;
		String maxDuration = null;
		String maxBandwidth = null;
		String dataSizeBytes = null;
		
		CoScheduleRequestField coScheReq = null;
		
		String inputXmlStr = inputName;
		InputSource inputSourceStr = new InputSource();
		inputSourceStr.setCharacterStream(new StringReader(inputXmlStr));
		
		try{
			DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
			DocumentBuilder builder = factory.newDocumentBuilder();
			//Document document = builder.parse( new File(inputName) ); //read from file
			Document document = builder.parse(inputSourceStr);  //read from string
			
            NodeList children = document.getChildNodes();
            /*
            for (int i = 0; i < children.getLength(); i++) {
               Node child = children.item(i);
               String nodeName = child.getNodeName();
               if (nodeName != null && nodeName.equalsIgnoreCase("optionalConstraint")) {
            	   optionalConsName = child.getAttributes().getNamedItem("category").getTextContent().trim();
            	   if(optionalConsName.equalsIgnoreCase("api-experiment-stornet")){
            		   children = child.getChildNodes();
            		   for(i = 0; i < children.getLength(); i++){
            			   child = children.item(i);
            			   nodeName = child.getNodeName();
            			   if(nodeName != null && nodeName.equalsIgnoreCase("value")){
            				   children = child.getChildNodes();
            				   for(i=0; i < children.getLength(); i++){
            					   child = children.item(i);
            					   nodeName = child.getNodeName();
            					   if(nodeName !=null && nodeName.equalsIgnoreCase("coScheduleRequest")){
            						   coScheduleRequestId = child.getAttributes().getNamedItem("id").getTextContent().trim();
            						   children = child.getChildNodes();
            						   for(i=0; i < children.getLength(); i++){
            							   child = children.item(i);
            							   nodeName = child.getNodeName();
            							   if(nodeName != null && nodeName.equalsIgnoreCase("startTime")){
            								   startTime = child.getTextContent().trim();
            							   }else if(nodeName != null && nodeName.equalsIgnoreCase("endTime")){
            								   endTime = child.getTextContent().trim();
            							   }else if(nodeName != null && nodeName.equalsIgnoreCase("minBandwidth")){
            								   minBandwidth = child.getTextContent().trim();
            							   }else if(nodeName != null && nodeName.equalsIgnoreCase("maxNumOfAltPaths")){
            								   maxNumOfAltPaths = child.getTextContent().trim();
            							   }else if(nodeName != null && nodeName.equalsIgnoreCase("bandwidthAvailabilityGraph")){
            								   bandwidthAvailabilityGraph = child.getTextContent().trim();
            							   }else if(nodeName != null && nodeName.equalsIgnoreCase("contiguousVlan")){
            								   contiguousVlan = child.getTextContent().trim();
            							   }else if(nodeName != null && nodeName.equalsIgnoreCase("maxDuration")){
            								   maxDuration = child.getTextContent().trim();
            							   }else if(nodeName != null && nodeName.equalsIgnoreCase("maxBandwidth")){
            								   maxBandwidth = child.getTextContent().trim();
            							   }else if(nodeName != null && nodeName.equalsIgnoreCase("dataSizeBytes")){
            								   dataSizeBytes = child.getTextContent().trim();
            							   }else{
            								   
            							   }
            						   }
            						   coScheReq = new CoScheduleRequestField();
            						   coScheReq.setCoScheduleRequestId(coScheduleRequestId);
            						   if(startTime!=null){
            							   coScheReq.setStartTime(Long.valueOf(startTime));
            						   }
            						   if(endTime!=null){
            							   coScheReq.setEndTime(Long.valueOf(endTime));
            						   }
            						   if(minBandwidth!=null){
            							   coScheReq.setMinBandwidth(Long.valueOf(minBandwidth));
            						   }
            						   if(maxNumOfAltPaths!=null){
            							   coScheReq.setMaxNumOfAltPaths(Integer.valueOf(maxNumOfAltPaths));
            						   }
            						   if(bandwidthAvailabilityGraph!=null){
            							   coScheReq.setBandwidthAvailabilityGraph(Boolean.valueOf(bandwidthAvailabilityGraph));
            						   }
            						   if(contiguousVlan!=null){
            							   coScheReq.setContiguousVlan(Boolean.valueOf(contiguousVlan));
            						   }
            						   if(maxDuration!=null){
            							   coScheReq.setMaxDuration(Integer.valueOf(maxDuration));
            						   }
            						   if(maxBandwidth!=null){
            							   coScheReq.setMaxBandwidth(Long.valueOf(maxBandwidth));
            						   }
            						   if(dataSizeBytes!=null){
            							   coScheReq.setDataSizeBytes(Long.valueOf(dataSizeBytes));
            						   }
            						   
            						   break;
            					   }
            				   }
            				   break;
            			   }            			
            		   }
            	   }
            	   break;
               }                             
            }
            */
            for(int i=0; i < children.getLength(); i++){
				   Node child = children.item(i);
				   String nodeName = child.getNodeName();
				   if(nodeName !=null && nodeName.equalsIgnoreCase("coScheduleRequest")){
					   coScheduleRequestId = child.getAttributes().getNamedItem("id").getTextContent().trim();
					   children = child.getChildNodes();
					   for(i=0; i < children.getLength(); i++){
						   child = children.item(i);
						   nodeName = child.getNodeName();
						   if(nodeName != null && nodeName.equalsIgnoreCase("startTime")){
							   startTime = child.getTextContent().trim();
						   }else if(nodeName != null && nodeName.equalsIgnoreCase("endTime")){
							   endTime = child.getTextContent().trim();
						   }else if(nodeName != null && nodeName.equalsIgnoreCase("minBandwidth")){
							   minBandwidth = child.getTextContent().trim();
						   }else if(nodeName != null && nodeName.equalsIgnoreCase("maxNumOfAltPaths")){
							   maxNumOfAltPaths = child.getTextContent().trim();
						   }else if(nodeName != null && nodeName.equalsIgnoreCase("bandwidthAvailabilityGraph")){
							   bandwidthAvailabilityGraph = child.getTextContent().trim();
						   }else if(nodeName != null && nodeName.equalsIgnoreCase("contiguousVlan")){
							   contiguousVlan = child.getTextContent().trim();
						   }else if(nodeName != null && nodeName.equalsIgnoreCase("maxDuration")){
							   maxDuration = child.getTextContent().trim();
						   }else if(nodeName != null && nodeName.equalsIgnoreCase("maxBandwidth")){
							   maxBandwidth = child.getTextContent().trim();
						   }else if(nodeName != null && nodeName.equalsIgnoreCase("dataSizeBytes")){
							   dataSizeBytes = child.getTextContent().trim();
						   }else{
							   
						   }
					   }
					   coScheReq = new CoScheduleRequestField();
					   coScheReq.setCoScheduleRequestId(coScheduleRequestId);
					   if(startTime!=null){
						   coScheReq.setStartTime(Long.valueOf(startTime));
					   }
					   if(endTime!=null){
						   coScheReq.setEndTime(Long.valueOf(endTime));
					   }
					   if(minBandwidth!=null){
						   coScheReq.setMinBandwidth((Long.valueOf(minBandwidth))*1000000);
					   }
					   if(maxNumOfAltPaths!=null){
						   coScheReq.setMaxNumOfAltPaths(Integer.valueOf(maxNumOfAltPaths));
					   }
					   if(bandwidthAvailabilityGraph!=null){
						   coScheReq.setBandwidthAvailabilityGraph(Boolean.valueOf(bandwidthAvailabilityGraph));
					   }
					   if(contiguousVlan!=null){
						   coScheReq.setContiguousVlan(Boolean.valueOf(contiguousVlan));
					   }
					   if(maxDuration!=null){
						   coScheReq.setMaxDuration(Integer.valueOf(maxDuration));
					   }
					   if(maxBandwidth!=null){
						   coScheReq.setMaxBandwidth((Long.valueOf(maxBandwidth))*1000000);
					   }
					   if(dataSizeBytes!=null){
						   coScheReq.setDataSizeBytes(Long.valueOf(dataSizeBytes));
					   }
					   
					   break;
				   }
			   }          
            
		}catch(ParserConfigurationException e){
			
		}catch(SAXException e){
			
		}catch(IOException e){
			
		}
		
		/*
		System.out.println("coScheduleRequestId=" + coScheduleRequestId);
		System.out.println("startTime=" + startTime);
		System.out.println("endTime=" + endTime);
		System.out.println("minBandwidth=" + minBandwidth);
		System.out.println("maxNumOfAltPaths=" + maxNumOfAltPaths);
		System.out.println("bandwidthAvailabilityGraph=" + bandwidthAvailabilityGraph);
		System.out.println("contiguousVlan=" + contiguousVlan);		
		System.out.println("maxDuration=" + maxDuration);
		System.out.println("maxBandwidth=" + maxBandwidth);
		System.out.println("dataSizeBytes=" + dataSizeBytes);
		*/
		//this.printReq(coScheReq);
		return coScheReq;
		

	}
	
	/*
	public static void main(String[] args) {
		// TODO Auto-generated method stub
		
		String fileName = "E:\\test.xml";
		Test1 testXml = new Test1();
		CoScheduleRequestField coScheReqField = testXml.readFile(fileName);
		if(coScheReqField!=null){
			testXml.printReq(coScheReqField);
		}
		
	
		Test1 testXmlOut = new Test1();
		CoScheduleReplyField coScheduleReply = testXmlOut.assignValue();
		BuildXml builder = new BuildXml();
		builder.generateXml(coScheduleReply);
		

	}
	*/


	protected void printReq(CoScheduleRequestField coScheReq){
		
		System.out.println("coScheduleRequestId=" + coScheReq.getCoScheduleRequestId());
		if(coScheReq.getStartTime()!=-1){
			System.out.println("startTime=" + coScheReq.getStartTime());
		}
		if(coScheReq.getEndTime()!=-1){
			System.out.println("endTime=" + coScheReq.getEndTime());
		}
		if(coScheReq.getMinBandwidth()!=-1){
			System.out.println("minBandwidth=" + coScheReq.getMinBandwidth());
		}
		if(coScheReq.getMaxNumOfAltPaths()!=-1){
			System.out.println("maxNumOfAltPaths=" + coScheReq.getMaxNumOfAltPaths());
		}
		System.out.println("bandwidthAvailabilityGraph=" + coScheReq.getBandwidthAvailabilityGraph());
		if(coScheReq.getContiguousVlan()!=null){
			System.out.println("contiguousVlan=" + coScheReq.getContiguousVlan());		
		}
		if(coScheReq.getMaxDuration()!=-1){
			System.out.println("maxDuration=" + coScheReq.getMaxDuration());
		}
		if(coScheReq.getMaxBandwidth()!=-1){
			System.out.println("maxBandwidth=" + coScheReq.getMaxBandwidth());
		}
		if(coScheReq.getDataSizeBytes()!=-1){
			System.out.println("dataSizeBytes=" + coScheReq.getDataSizeBytes());
		}
	}
	
	protected CoScheduleReplyField assignValue(){
		
		CoScheduleReplyField coScheduleReply = new CoScheduleReplyField();
		coScheduleReply.setId("schedule-124423245-option-1");
		List<CoSchedulePathField> coSchedulePath = coScheduleReply.getCoSchedulePath();
		
		//set first coSchedulePath
		CoSchedulePathField coSchedulePathField = new CoSchedulePathField();
		coSchedulePathField.setId("path-124423245-1");
		PathInfoField pathInfo = new PathInfoField();
		coSchedulePathField.setPathInfoField(pathInfo);
		BagInfoField bagInfo = new BagInfoField();
		coSchedulePathField.setBagInfoField(bagInfo);
		
		//set path info
		pathInfo.setPathId("path-124423245-1");		
		List<HopField> hop = pathInfo.getHop();
		//set hops
		HopField hopField = new HopField();
		hopField.setHopId("79083930-ae60-4d58-8694-1362668b8782");
		LinkField link = new LinkField();
		link.setLinkId("urn:ogf:network:domain=testdomain-2.net:node=node-3:port=port-5:link=link-1");
		link.setSwitchingcapType("l2sc");
		link.setEncodingType("ethernet");
		link.setVlanRangeAvailability("any");
		hopField.setLink(link);
		hop.add(hopField);
		//end set hops
		
		//set second hop
		hopField = new HopField();
		hopField.setHopId("a4bdbcaf-eaea-45cd-93dc-96f72212877b");
		link = new LinkField();
		link.setLinkId("urn:ogf:network:domain=testdomain-1.net:node=node-3:port=port-5:link=link-1");
		link.setSwitchingcapType("l2sc");
		link.setEncodingType("ethernet");
		link.setVlanRangeAvailability("1000");
		hopField.setLink(link);
		hop.add(hopField);
		//end set second hop
		
		//set bag info
		List<BagSegmentField> bagSegment = bagInfo.getBagSegment();
		//set set bag segments
		BagSegmentField bagSegmentField = new BagSegmentField();
		bagSegmentField.setSegmentBandwidth(100);
		bagSegmentField.setSegmentStartTime(1303331764);
		bagSegmentField.setSegmentEndTime(1303335364);
		bagSegment.add(bagSegmentField);
		//end set bag segments
		
		//set second bag segment
		bagSegmentField = new BagSegmentField();
		bagSegmentField.setSegmentBandwidth(600);
		bagSegmentField.setSegmentStartTime(1303335364);
		bagSegmentField.setSegmentEndTime(1303342564);
		bagSegment.add(bagSegmentField);
		//end set second bag segment
		
		//set third bag segment
		bagSegmentField = new BagSegmentField();
		bagSegmentField.setSegmentBandwidth(100);
		bagSegmentField.setSegmentStartTime(1303342564);
		bagSegmentField.setSegmentEndTime(1303367764);
		bagSegment.add(bagSegmentField);
		//end set third bag segment
		
		coSchedulePath.add(coSchedulePathField);
		
		//end set first coSchedulePath
		
		
		//set second coSchedulePath
		coSchedulePathField = new CoSchedulePathField();
		coSchedulePathField.setId("path-124423245-2");
		pathInfo = new PathInfoField();
		coSchedulePathField.setPathInfoField(pathInfo);
		bagInfo = new BagInfoField();
		coSchedulePathField.setBagInfoField(bagInfo);
		
		//set path info
		pathInfo.setPathId("path-124423245-2");		
		hop = pathInfo.getHop();
		//set hops
		hopField = new HopField();
		hopField.setHopId("93079083-ae60-4d58-8694-1368b8726682");
		link = new LinkField();
		link.setLinkId("urn:ogf:network:domain=testdomain-2.net:node=node-1:port=port-3:link=link-2");
		link.setSwitchingcapType("l2sc");
		link.setEncodingType("ethernet");
		link.setVlanRangeAvailability("2000");
		hopField.setLink(link);
		hop.add(hopField);
		//end set hops
		
		//set second hop
		hopField = new HopField();
		hopField.setHopId("bdbca4af-eaea-45cd-93dc-87f7227b9612");
		link = new LinkField();
		link.setLinkId("urn:ogf:network:domain=testdomain-1.net:node=node-2:port=port-1:link=link-1");
		link.setSwitchingcapType("l2sc");
		link.setEncodingType("ethernet");
		link.setVlanRangeAvailability("any");
		hopField.setLink(link);
		hop.add(hopField);
		//end set second hop
		
		//set bag info
		bagSegment = bagInfo.getBagSegment();
		//set set bag segments
		bagSegmentField = new BagSegmentField();
		bagSegmentField.setSegmentBandwidth(200);
		bagSegmentField.setSegmentStartTime(1303333450);
		bagSegmentField.setSegmentEndTime(1303333950);
		bagSegment.add(bagSegmentField);
		//end set bag segments
		
		//set second bag segment
		bagSegmentField = new BagSegmentField();
		bagSegmentField.setSegmentBandwidth(100);
		bagSegmentField.setSegmentStartTime(1303335312);
		bagSegmentField.setSegmentEndTime(1303342512);
		bagSegment.add(bagSegmentField);
		//end set second bag segment
		
		//set third bag segment
		bagSegmentField = new BagSegmentField();
		bagSegmentField.setSegmentBandwidth(800);
		bagSegmentField.setSegmentStartTime(1303342535);
		bagSegmentField.setSegmentEndTime(1303367735);
		bagSegment.add(bagSegmentField);
		//end set third bag segment
		
		coSchedulePath.add(coSchedulePathField);
		
		//end set second coSchedulePath
		
		
		
		//set third coSchedulePath
		coSchedulePathField = new CoSchedulePathField();
		coSchedulePathField.setId("path-124423245-3");
		pathInfo = new PathInfoField();
		coSchedulePathField.setPathInfoField(pathInfo);
		bagInfo = new BagInfoField();
		coSchedulePathField.setBagInfoField(bagInfo);
		
		//set path info
		pathInfo.setPathId("path-124423245-3");		
		hop = pathInfo.getHop();
		//set hops
		hopField = new HopField();
		hopField.setHopId("98370930-ae60-4d58-8694-7266821368b8");
		link = new LinkField();
		link.setLinkId("urn:ogf:network:domain=testdomain-1.net:node=node-3:port=port-3:link=link-5");
		link.setSwitchingcapType("l2sc");
		link.setEncodingType("ethernet");
		link.setVlanRangeAvailability("3000");
		hopField.setLink(link);
		hop.add(hopField);
		//end set hops
		
		//set second hop
		hopField = new HopField();
		hopField.setHopId("ca4afbdb-eaea-45cd-93dc-27b961287f72");
		link = new LinkField();
		link.setLinkId("urn:ogf:network:domain=testdomain-1.net:node=node-2:port=port-3:link=link-1");
		link.setSwitchingcapType("l2sc");
		link.setEncodingType("ethernet");
		link.setVlanRangeAvailability("any");
		hopField.setLink(link);
		hop.add(hopField);
		//end set second hop
		
		//set bag info
		bagSegment = bagInfo.getBagSegment();
		//set set bag segments
		bagSegmentField = new BagSegmentField();
		bagSegmentField.setSegmentBandwidth(700);
		bagSegmentField.setSegmentStartTime(1303333496);
		bagSegmentField.setSegmentEndTime(1303333996);
		bagSegment.add(bagSegmentField);
		//end set bag segments
		
		//set second bag segment
		bagSegmentField = new BagSegmentField();
		bagSegmentField.setSegmentBandwidth(200);
		bagSegmentField.setSegmentStartTime(1303335369);
		bagSegmentField.setSegmentEndTime(1303342569);
		bagSegment.add(bagSegmentField);
		//end set second bag segment
		
		//set third bag segment
		bagSegmentField = new BagSegmentField();
		bagSegmentField.setSegmentBandwidth(300);
		bagSegmentField.setSegmentStartTime(1303342559);
		bagSegmentField.setSegmentEndTime(1303367759);
		bagSegment.add(bagSegmentField);
		//end set third bag segment
		
		coSchedulePath.add(coSchedulePathField);
		
		//end set third coSchedulePath
		
		return coScheduleReply;
	}

}
