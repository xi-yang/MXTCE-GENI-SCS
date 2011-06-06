package net.es.oscars.pce.tce;

public class ReplyLinkContent {
	
	protected String id;
	protected String name;
	protected String switchingType;
	protected String encodingType;
	protected String assignedVlanTags;
	protected String availableVlanTags;
	protected String suggestedVlanTags;
	protected int mtu;
	protected boolean vlanTranslation;
	protected int capacity;
	
	public String getId(){
		return this.id;
	}
	
	public void setId(String value){
		this.id = value;
	}
	
	public String getName(){
		return this.name;
	}
	
	public void setName(String value){
		this.name = value;
	}
	
	public String getSwitchingType(){
		return this.switchingType;
	}
	
	public void setSwitchingType(String value){
		this.switchingType = value;
	}
	
	public String getEncodingType(){
		return this.encodingType;
	}
	
	public void setEncodingType(String value){
		this.encodingType = value;
	}
	
	public String getAssignedVlanTags(){
		return this.assignedVlanTags;
	}
	
	public void setAssignedVlanTags(String value){
		this.assignedVlanTags = value;
	}
	
	public String getAvailableVlanTags(){
		return this.availableVlanTags;
	}
	
	public void setAvailableVlanTags(String value){
		this.availableVlanTags = value;
	}
	
	public String getSuggestedVlanTags(){
		return this.suggestedVlanTags;
	}
	
	public void setSuggestedVlanTags(String value){
		this.suggestedVlanTags = value;
	}
	
	public int getMtu(){
		return this.mtu;
	}
	
	public void setMtu(int value){
		this.mtu = value;
	}
	
	public boolean getVlanTranslation(){
		return this.vlanTranslation;
	}
	
	public void setVlanTranslation(boolean value){
		this.vlanTranslation = value;
	}
	
	public int getCapacity(){
		return this.capacity;
	}
	
	public void setCapacity(int value){
		this.capacity = value;
	}

}
