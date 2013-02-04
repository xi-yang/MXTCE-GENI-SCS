package net.es.oscars.pce.tce.optionalConstraint.stornet;

public class CoScheduleRequestField {
	
	protected String coScheduleRequestId;
	protected long startTime;
	protected long endTime;
	protected long minBandwidth;
	protected int maxNumOfAltPaths;
	protected boolean bandwidthAvailabilityGraph; 
	protected Boolean contiguousVlan;
	protected int maxDuration;
	protected long maxBandwidth;
	protected long dataSizeBytes;
	
	CoScheduleRequestField(){
		//initial variables
		this.startTime = -1;
		this.endTime = -1;
		this.minBandwidth = -1;
		this.maxNumOfAltPaths = -1;
		this.contiguousVlan = null;
		this.maxDuration = -1;
		this.maxBandwidth = -1;
		this.dataSizeBytes = -1;
	}
	
	public String getCoScheduleRequestId(){
		return this.coScheduleRequestId;
	}
	
	public void setCoScheduleRequestId(String value){
		this.coScheduleRequestId = value;
	}
	
	public long getStartTime(){
		return this.startTime;
	}
	
	public void setStartTime(long value){
		this.startTime = value;
	}
	
	public long getEndTime(){
		return this.endTime;
	}
	
	public void setEndTime(long value){
		this.endTime = value;
	}
	
	public long getMinBandwidth(){
		return this.minBandwidth;
	}
	
	public void setMinBandwidth(long value){
		this.minBandwidth = value;
	}
	
	public int getMaxNumOfAltPaths(){
		return this.maxNumOfAltPaths;
	}
	
	public void setMaxNumOfAltPaths(int value){
		this.maxNumOfAltPaths = value;
	}
	
	public boolean getBandwidthAvailabilityGraph(){
		return this.bandwidthAvailabilityGraph;
	}
	
	public void setBandwidthAvailabilityGraph(boolean value){
		this.bandwidthAvailabilityGraph = value;
	}
	
	public Boolean getContiguousVlan(){
		return this.contiguousVlan;
	}
	
	public void setContiguousVlan(Boolean value){
		this.contiguousVlan = value;
	}
	
	public int getMaxDuration(){
		return this.maxDuration;
	}
	
	public void setMaxDuration(int value){
		this.maxDuration = value;
	}
	
	public long getMaxBandwidth(){
		return this.maxBandwidth;
	}
	
	public void setMaxBandwidth(long value){
		this.maxBandwidth = value;
	}
	
	public long getDataSizeBytes(){
		return this.dataSizeBytes;
	}
	
	public void setDataSizeBytes(long value){
		this.dataSizeBytes = value;
	}

}
