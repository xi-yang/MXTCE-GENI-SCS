package net.es.oscars.pce.tce.optionalConstraint.stornet;

public class CoSchedulePathField {
	
	protected String id;
	protected PathInfoField pathInfo;
	protected BagInfoField bagInfo;
	
	public String getId(){
		return this.id;
	}
	
	public void setId(String value){
		this.id = value;
	}
	
	public PathInfoField getPathInfoField(){
		return this.pathInfo;
	}
	
	public void setPathInfoField(PathInfoField value){
		this.pathInfo = value;
	}
	
	public BagInfoField getBagInfoField(){
		return this.bagInfo;
	}
	
	public void setBagInfoField(BagInfoField value){
		this.bagInfo = value;
	}

}
