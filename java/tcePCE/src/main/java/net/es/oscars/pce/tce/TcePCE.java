package net.es.oscars.pce.tce;

import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.HashMap;

import org.apache.log4j.Logger;
import org.ogf.schema.network.topology.ctrlplane.CtrlPlaneDomainContent;
import org.ogf.schema.network.topology.ctrlplane.CtrlPlaneHopContent;
import org.ogf.schema.network.topology.ctrlplane.CtrlPlaneLinkContent;
import org.ogf.schema.network.topology.ctrlplane.CtrlPlaneNodeContent;
import org.ogf.schema.network.topology.ctrlplane.CtrlPlanePathContent;
import org.ogf.schema.network.topology.ctrlplane.CtrlPlanePortContent;
import org.ogf.schema.network.topology.ctrlplane.CtrlPlaneTopologyContent;

import net.es.oscars.api.soap.gen.v06.ListReply;
import net.es.oscars.api.soap.gen.v06.ListRequest;
import net.es.oscars.api.soap.gen.v06.ResDetails;
import net.es.oscars.common.soap.gen.AuthConditionType;
import net.es.oscars.common.soap.gen.AuthConditions;
import net.es.oscars.logging.ErrSev;
import net.es.oscars.logging.OSCARSNetLogger;
import net.es.oscars.pce.PCEMessage;
import net.es.oscars.pce.soap.gen.v06.PCEDataContent;
import net.es.oscars.utils.clients.RMClient;
import net.es.oscars.utils.sharedConstants.StateEngineValues;
import net.es.oscars.utils.soap.OSCARSServiceException;
import net.es.oscars.utils.topology.NMWGParserUtil;

import java.net.*;
import java.io.*;

public class TcePCE {
    private Logger log = Logger.getLogger(TcePCE.class);
    private String rmWsdl;
    private String rmUrl;
    //private RMPortType rmClient;
    private RMClient rmClient;
    
    final private String[] STATUSES = {
            StateEngineValues.ACTIVE, StateEngineValues.INPATHCALCULATION,
            StateEngineValues.INSETUP, StateEngineValues.INTEARDOWN, 
            StateEngineValues.INMODIFY, StateEngineValues.INCOMMIT, 
            StateEngineValues.COMMITTED, StateEngineValues.RESERVED
            };
    final private double MBPS_DENOM = 1000000.0;
    
    
    public TcePCE(String rmUrl, String rmWsdl) throws OSCARSServiceException{
        this.rmClient = null;
        this.rmUrl = rmUrl;
        if(rmWsdl == null){
            this.rmWsdl = rmUrl+"?wsdl";
        }else{
            this.rmWsdl = rmWsdl;
        }
    }
    
    public PCEDataContent calculatePath(PCEMessage query) throws OSCARSServiceException, MalformedURLException, IOException{
        return this.calculatePath(query, null);
    }
    
    
    public PCEDataContent calculatePath(PCEMessage query, OSCARSNetLogger netLogger) throws OSCARSServiceException, MalformedURLException, IOException{
        synchronized(this){
            if(this.rmClient == null){
                //this.rmClient = RMClient.getClient(new URL(this.rmUrl), new URL(this.rmWsdl)).getPortType();
                this.rmClient = RMClient.getClient(new URL(this.rmUrl), new URL(this.rmWsdl));
            }
        }
        
        byte[] apiMsg;
        
        CreateApiMsg createMsg = new CreateApiMsg(query);
        
        createMsg.encodeApiMsg();
        
        apiMsg = createMsg.getEncodedApiMsg();
        
        
		String host="localhost";
		int tcePort=2089;
		Socket socket;		
		
	    socket=new Socket(host,tcePort);
	    
		OutputStream f1 =socket.getOutputStream();
		InputStream r1 =socket.getInputStream();
		
		f1.write(apiMsg);
		
		ByteArrayOutputStream buffer = new ByteArrayOutputStream();
		byte[] buff = new byte[1024];
		int length = -1;
		int totalReadLength = 0;
		int messageLength = -1;
		byte[] replyApiBuff;
		while((length = r1.read(buff))!=-1){
			
			totalReadLength = totalReadLength + length;			
			buffer.write(buff, 0, length);
			if(totalReadLength >= 24){
				messageLength = this.getLength(buffer.toByteArray());
			}
			if(totalReadLength == messageLength + 24){
				break;
			}
		}		
		
		ReplyMessageContent replyMessage = null;
		RetrieveReply retrieveMsg = new RetrieveReply();
		replyApiBuff = buffer.toByteArray();
		retrieveMsg.checkApiMsg(replyApiBuff);
		replyMessage = retrieveMsg.decodeReplyMessage(replyApiBuff);

		socket.close();
		
		WriteResult writeRes = new WriteResult(query);

		writeRes.writeComputeRes(replyMessage);        
		
        PCEDataContent pceData = query.getPCEDataContent();  //pceData has been updated by method writeComputeRes() if path compute result is succeed

        return pceData;
        
        
    }
    
    public PCEDataContent commitPath(PCEMessage query) throws OSCARSServiceException{

        PCEDataContent pceData = query.getPCEDataContent();
        
        return pceData;
    }
    
	private int getLength(byte[] buff){
		int result = 0;
		int value = 0;
		int offset = 2;
		
		for(int i=0;i<2;i++){
			value = 0xFF & buff[offset++];
			result = (result << 8) | value;
		}
		
		return result;
	}

}
