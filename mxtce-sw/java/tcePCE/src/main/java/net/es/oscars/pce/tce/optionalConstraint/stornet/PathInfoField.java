package net.es.oscars.pce.tce.optionalConstraint.stornet;

import java.util.ArrayList;
import java.util.List;

public class PathInfoField {
	
	protected String pathId;
	protected List<HopField> hop;
	
	public String getPathId(){
		return this.pathId;
	}
	
	public void setPathId(String value){
		this.pathId = value;
	}
	
	public List<HopField> getHop(){
		if(hop==null){
			hop = new ArrayList<HopField>();
		}
		return this.hop;
	}

}
