package net.es.oscars.pce.tce.optionalConstraint.stornet;

public class BagSegmentField {
	
	protected int segmentBandwidth;
	protected int segmentStartTime;
	protected int segmentEndTime;
	
	public int getSegmentBandwidth(){
		return this.segmentBandwidth;
	}
	
	public void setSegmentBandwidth(int value){
		this.segmentBandwidth = value;
	}
	
	public int getSegmentStartTime(){
		return this.segmentStartTime;
	}
	
	public void setSegmentStartTime(int value){
		this.segmentStartTime = value;
	}
	
	public int getSegmentEndTime(){
		return this.segmentEndTime;
	}
	
	public void setSegmentEndTime(int value){
		this.segmentEndTime = value;
	}

}
