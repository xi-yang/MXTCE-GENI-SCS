package net.es.oscars.pce.tce;

public class ReplyBagSegmentContent {
	
	protected int bandwidth;
	protected int startTime;
	protected int endTime;
	
	public int getBandwidth(){
		return this.bandwidth;
	}
	
	public void setBandwidth(int value){
		this.bandwidth = value;
	}
	
	public int getStartTime(){
		return this.startTime;
	}
	
	public void setStartTime(int value){
		this.startTime = value;
	}
	
	public int getEndTime(){
		return this.endTime;
	}
	
	public void setEndTime(int value){
		this.endTime = value;
	}

}
