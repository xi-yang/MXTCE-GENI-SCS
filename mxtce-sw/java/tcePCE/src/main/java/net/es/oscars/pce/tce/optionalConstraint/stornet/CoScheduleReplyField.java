package net.es.oscars.pce.tce.optionalConstraint.stornet;

import java.util.ArrayList;
import java.util.List;


public class CoScheduleReplyField {
	
	protected String id;
	protected List<CoSchedulePathField> coSchedulePath;
	
	public List<CoSchedulePathField> getCoSchedulePath(){
		if(coSchedulePath==null){
			coSchedulePath = new ArrayList<CoSchedulePathField>();
		}
		return this.coSchedulePath;
	}
	
	public String getId(){
		return this.id;
	}
	
	public void setId(String value){
		this.id = value;
	}

}
