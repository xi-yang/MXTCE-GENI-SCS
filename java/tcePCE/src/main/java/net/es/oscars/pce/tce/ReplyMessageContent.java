package net.es.oscars.pce.tce;


public class ReplyMessageContent {
	
	protected ReplyPathContent path;
	protected String gri;
	protected String errorMsg;
	protected ReplyCoSchedulePathContent coSchedulePath;
	
	public ReplyMessageContent(){
		this.errorMsg = null;
		this.path = null;
		this.coSchedulePath = null;
	}
	
	public ReplyPathContent getReplyPathContent(){
		return this.path;
	}
	
	public void setReplyPathContent(ReplyPathContent value){
		this.path = value;
	}
	
	public String getGri(){
		return this.gri;
	}
	
	public void setGri(String value){
		this.gri = value;
	}
	
	public String getErrorMessage(){
		return this.errorMsg;
	}
	
	public void setErrorMessage(String value){
		this.errorMsg = value;
	}
	
	public ReplyCoSchedulePathContent getCoSchedulePath(){
		return this.coSchedulePath;
	}
	
	public void setCoScheduleReply(ReplyCoSchedulePathContent value){
		this.coSchedulePath = value;
	}

}
