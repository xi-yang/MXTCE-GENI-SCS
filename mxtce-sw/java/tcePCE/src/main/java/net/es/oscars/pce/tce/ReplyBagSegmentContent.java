package net.es.oscars.pce.tce;

public class ReplyBagSegmentContent {
	
	protected long bandwidth;
	protected int startTime;
	protected int endTime;
	
	public long getBandwidth(){
		return this.bandwidth;
	}
	
	public void setBandwidth(long value){
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
