package net.es.oscars.pce.tce;

import java.util.ArrayList;
import java.util.List;

public class ReplyPathContent {
	
	protected List<ReplyLinkContent> links;
	protected String id;
	protected List<ReplyBagSegmentContent> bags;
	
	public List<ReplyLinkContent> getReplyLinkContent(){
		if(links==null){
			links = new ArrayList<ReplyLinkContent>();
		}
		return this.links;
	}
	
	public String getId(){
		return this.id;
	}
	
	public void setId(String value){
		this.id = value;
	}
	
	public List<ReplyBagSegmentContent> getReplyBagSegmentContent(){
		if(bags==null){
			bags = new ArrayList<ReplyBagSegmentContent>();
		}
		return this.bags;
	}

}
