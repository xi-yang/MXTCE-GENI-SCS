package net.es.oscars.pce.tce.optionalConstraint.stornet;

import java.util.ArrayList;
import java.util.List;

public class BagInfoField {

	protected List<BagSegmentField> bagSegment;
	
	public List<BagSegmentField> getBagSegment(){
		if(bagSegment==null){
			bagSegment = new ArrayList<BagSegmentField>();
		}
		return this.bagSegment;
	}
	

}
