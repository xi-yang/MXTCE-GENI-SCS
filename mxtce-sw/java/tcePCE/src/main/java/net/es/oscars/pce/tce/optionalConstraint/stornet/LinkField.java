package net.es.oscars.pce.tce.optionalConstraint.stornet;

public class LinkField {
	
	protected String linkId;
	protected String switchingcapType;
	protected String encodingType;
	protected String vlanRangeAvailability;
	
	public String getLinkId(){
		return this.linkId;
	}
	
	public void setLinkId(String value){
		this.linkId = value;
	}
	
	public String getSwitchingcapType(){
		return this.switchingcapType;
	}
	
	public void setSwitchingcapType(String value){
		this.switchingcapType = value;
	}
	
	public String getEncodingType(){
		return this.encodingType;
	}
	
	public void setEncodingType(String value){
		this.encodingType = value;
	}
	
	public String getVlanRangeAvailability(){
		return this.vlanRangeAvailability;
	}
	
	public void setVlanRangeAvailability(String value){
		this.vlanRangeAvailability = value;
	}

}
