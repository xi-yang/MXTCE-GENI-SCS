/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package net.es.oscars.pce.tce.client;

import org.apache.log4j.Logger;
import net.es.oscars.logging.ErrSev;
import net.es.oscars.logging.ModuleName;
import net.es.oscars.logging.OSCARSNetLogger;
import net.es.oscars.pce.soap.gen.v06.AggregatorCreateCommitContent;
import net.es.oscars.pce.soap.gen.v06.AggregatorModifyCommitContent;
import net.es.oscars.pce.soap.gen.v06.AggregatorCreateContent;
import net.es.oscars.pce.soap.gen.v06.AggregatorModifyContent;
import net.es.oscars.pce.soap.gen.v06.AggregatorCancelContent;
import net.es.oscars.pce.soap.gen.v06.PCECancelContent;
import net.es.oscars.pce.soap.gen.v06.PCECancelReplyContent;
import net.es.oscars.pce.soap.gen.v06.PCECreateCommitContent;
import net.es.oscars.pce.soap.gen.v06.PCECreateCommitReplyContent;
import net.es.oscars.pce.soap.gen.v06.PCEModifyCommitContent;
import net.es.oscars.pce.soap.gen.v06.PCEModifyCommitReplyContent;
import net.es.oscars.pce.soap.gen.v06.PCECreateContent;
import net.es.oscars.pce.soap.gen.v06.PCEModifyContent;
import net.es.oscars.pce.soap.gen.v06.PCEModifyReplyContent;
import net.es.oscars.pce.soap.gen.v06.PCEReplyContent;
import net.es.oscars.pce.soap.gen.v06.PCEPortType;
import net.es.oscars.pce.soap.gen.v06.PCEDataContent;
import net.es.oscars.pce.soap.gen.v06.PCEError;

import net.es.oscars.utils.sharedConstants.PCERequestTypes;
import net.es.oscars.utils.sharedConstants.ErrorCodes;


/**
 *
 * @author xyang
 */

    @javax.jws.WebService(
                      serviceName = "PCEService",
                      portName = "PCEPortType",
                      targetNamespace = "http://oscars.es.net/OSCARS/PCE/20090922",
                      endpointInterface = "net.es.oscars.pce.soap.gen.v06.PCEPortType")
 @javax.xml.ws.BindingType(value = "http://www.w3.org/2003/05/soap/bindings/HTTP/")
public class TCERuntimeSoapHandler implements PCEPortType {

    private static final Logger LOG = Logger.getLogger(TCERuntimeSoapHandler.class.getName());

    private TCECallbackHandler callbackHandler = null;

    public TCECallbackHandler getCallbackHandler() {
        return callbackHandler;
    }

    public void setCallbackHandler(TCECallbackHandler callbackHandler) {
        this.callbackHandler = callbackHandler;
    }
    
    /**
     * Handles the pceReply message sent from a PCE or an aggregator when it completes a PCECreate request. 
     * It calls the PCE/AggProxyAction that sent the PCEcreate message to process the replyData.
     * If an error was returned, the proxyAction and the reservation will be set to FAILED. 
     * If no error was returned, the proxyAction will do the next thing on its list.
     */
    public void pceReply(PCEReplyContent pceReply) { 
        String method = "TCERuntimeSoapHandler.pceReply";
        OSCARSNetLogger netLogger = OSCARSNetLogger.getTlogger();
        String transId = pceReply.getMessageProperties().getGlobalTransactionId();
        netLogger.init("TCERuntime", transId);
        netLogger.setGRI(pceReply.getGlobalReservationId());
        String pceName = pceReply.getPceName();
        LOG.debug(netLogger.start(method, " from " + pceName));

        handleReply( method, 
                     netLogger,
                     pceReply.getGlobalReservationId(),
                     transId,
                     pceReply.getPceName(), 
                     pceReply.getPceData(), 
                     pceReply.getPceError(),
                     ErrorCodes.PCE_CREATE_FAILED);
    }
 
        /**
     * Handles the pceCreateCommitReply message sent from a PCE once it has completed a PCECreateCommit.
     * If it did not get an error it calls the pce action that sent the commit to 
     * do the next thing on its list. If there was an error it sets the pce action to fail.
     */
    public void pceCreateCommitReply(PCECreateCommitReplyContent pceCreateCommitReply)  {
        String method = "TCERuntimeSoapHandler.pceCreateCommitReply";
        String transId = pceCreateCommitReply.getMessageProperties().getGlobalTransactionId();
        OSCARSNetLogger netLogger = OSCARSNetLogger.getTlogger();
        netLogger.init("TCERuntime", transId);
        String globalReservationId = pceCreateCommitReply.getGlobalReservationId();
        netLogger.setGRI(globalReservationId);
        String pceName = pceCreateCommitReply.getPceName();
        PCEDataContent pceDataContent = pceCreateCommitReply.getPceData();
        PCEError pceError = pceCreateCommitReply.getPceError();
        LOG.debug(netLogger.start(method, " from " + pceName));
        handleReply( method, netLogger, globalReservationId, transId, pceName,
                     pceDataContent, pceError,ErrorCodes.PCE_COMMIT_FAILED);
    }
    
     /**
     * Handles the pceModifyCommitReply message sent from a PCE once it has completed a PCEModifyCommit.
     * If it did not get an error it calls the pce action that sent the commit to
     * do the next thing on its list. If there was an error it sets the pce action to fail.
     */
    public void pceModifyCommitReply(PCEModifyCommitReplyContent pceModifyCommitReply)  {
        String method = "TCERuntimeSoapHandler.pceModifyCommitReply";
        OSCARSNetLogger netLogger = OSCARSNetLogger.getTlogger();
        String transId = pceModifyCommitReply.getMessageProperties().getGlobalTransactionId();
        netLogger.init("TCERuntime", transId);
        String globalReservationId = pceModifyCommitReply.getGlobalReservationId();
        netLogger.setGRI(globalReservationId);
        String pceName = pceModifyCommitReply.getPceName();
        PCEDataContent pceDataContent = pceModifyCommitReply.getPceData();
        PCEError pceError = pceModifyCommitReply.getPceError();
        LOG.debug(netLogger.start(method, " from " + pceName));
        handleReply( method, netLogger, globalReservationId, transId, pceName,
                     pceDataContent, pceError, ErrorCodes.PCE_MODIFY_COMMIT_FAILED);
    }

    /**
     * Handles the pceModifyReply message sent from a PCE once it has completed a PCEModify. 
     * If it did not get an error it calls the pce action that sent the commit to 
     * do the next thing on its list. If there was an error it sets the pce action to fail.
     */
    public void pceModifyReply(PCEModifyReplyContent pceModifyReply) { 
        String method = "TCERuntimeSoapHandler.pceModifyReply";
        OSCARSNetLogger netLogger = OSCARSNetLogger.getTlogger();
        String transId = pceModifyReply.getMessageProperties().getGlobalTransactionId();
        netLogger.init("TCERuntime", transId);
        String globalReservationId = pceModifyReply.getGlobalReservationId();
        netLogger.setGRI(globalReservationId);
        String pceName = pceModifyReply.getPceName();
        LOG.debug(netLogger.start(method, " from " + pceName));
        PCEDataContent pceDataContent = pceModifyReply.getPceData();
        PCEError pceError = pceModifyReply.getPceError();
        handleReply( method, netLogger, globalReservationId, transId, pceName,
                    pceDataContent, pceError, ErrorCodes.PCE_MODIFY_FAILED);
    }

    
    public void pceCancelReply(PCECancelReplyContent pceCancelReply)  { 

        String method = "TCERuntimeSoapHandler.pceCancelReply";
        OSCARSNetLogger netLogger = OSCARSNetLogger.getTlogger();
        String transId = pceCancelReply.getMessageProperties().getGlobalTransactionId();
        netLogger.init("TCERuntime", transId);
        String globalReservationId = pceCancelReply.getGlobalReservationId();
        String pceName = pceCancelReply.getPceName();
        LOG.debug(netLogger.start(method, " from " + pceName));
        netLogger.setGRI(globalReservationId);
        PCEDataContent pceDataContent = pceCancelReply.getPceData();
        PCEError pceError = pceCancelReply.getPceError();
        handleReply(method, netLogger, globalReservationId, transId, pceName,
                    pceDataContent, pceError, ErrorCodes.PCE_CANCEL_FAILED);
    }

    /* provides boiler plate for handling replies.
     * Calls the pceProxyAction processReply method which will send the resultData
     * to the next agg/pce in the graph
     */
    @SuppressWarnings("static-access")
    private void handleReply(String method,
                             OSCARSNetLogger netLogger,
                             String globalReservationId,
                             String transId,
                             String pceName,
                             PCEDataContent pceDataContent,
                             PCEError pceError,
                             String errorCode )
                throws RuntimeException {

        String proxyRequestType = this.getRequestType(method);
        if (proxyRequestType == null) {
            LOG.error(netLogger.error(method,ErrSev.MINOR,"Invalid request:null"));
            // This cannot happen, but this is a safety code.
            throw new RuntimeException("Invalid request");
        }
        
        //TODO? processReply() local workflow handling ? 
        
        synchronized(callbackHandler.lock) {
            callbackHandler.handleReply(method, globalReservationId, transId, pceDataContent, pceError, errorCode);
            // callbackHandler.processReply(...)
        }
    }
    
    private String getRequestType (String method) {

        if ("TCERuntimeSoapHandler.pceReply".equals(method)) {
            return PCERequestTypes.PCE_CREATE;
        } else if ("TCERuntimeSoapHandler.pceCreateCommitReply".equals(method)) {
            return PCERequestTypes.PCE_CREATE_COMMIT;
        } else if ("TCERuntimeSoapHandler.pceModifyCommitReply".equals(method)) {
            return PCERequestTypes.PCE_MODIFY_COMMIT;
        } else if ("TCERuntimeSoapHandler.pceModifyReply".equals(method)) {
            return PCERequestTypes.PCE_MODIFY;
        } else if ("TCERuntimeSoapHandler.pceCancelReply".equals(method)) {
            return PCERequestTypes.PCE_CANCEL;
        }
        return null;
    }

    /**
     *  pceCreate implemented in PCE Service. TCERuntime only handles replies.
     */
    public void pceCreate(PCECreateContent pceCreate)   { 
        String event = PCERequestTypes.PCE_CREATE;
        OSCARSNetLogger netLogger = OSCARSNetLogger.getTlogger();
        String transId = pceCreate.getMessageProperties().getGlobalTransactionId();
        netLogger.init("TCERuntime", transId);
        LOG.error(netLogger.error(event,ErrSev.MINOR,"Executing operation pceCreate - Should not happen"));
        // The PCE Runtime is not expected to receive any PCE Create. Only replies.
        throw new RuntimeException("PCE Runtime received a PCECreate.");
    }

    /**
     *  aggregatorCreate implemented in PCE Service. TCERuntime only handles replies.
     */
    public void aggregatorCreate(AggregatorCreateContent aggregatorCreate) { 
        String event = PCERequestTypes.AGGREGATOR_CREATE;
        OSCARSNetLogger netLogger = OSCARSNetLogger.getTlogger();
        String transId = aggregatorCreate.getMessageProperties().getGlobalTransactionId();
        netLogger.init("TCERuntime", transId);
        LOG.error(netLogger.error(event,ErrSev.MINOR,"Executing operation aggregatorCreate - Should not happen"));
        // The PCE Runtime is not expected to receive any queries. Only replies.
        throw new RuntimeException("PCE Runtime received a aggregatorCreate.");
    }

    /**
     *  aggregatorModify implemented in PCE Service. TCERuntime only handles replies.
     */
    public void aggregatorModify(AggregatorModifyContent aggregatorModify )  { 

        String event = PCERequestTypes.AGGREGATOR_MODIFY;
        OSCARSNetLogger netLogger = OSCARSNetLogger.getTlogger();
        String transId = aggregatorModify.getMessageProperties().getGlobalTransactionId();
        netLogger.init("TCERuntime", transId);
        LOG.error(netLogger.error(event,ErrSev.MINOR,"Executing operation aggregatorModify - Should not happen"));
        // The PCE Runtime is not expected to receive any queries. Only replies.
        throw new RuntimeException("PCE Runtime received a aggregatorModify.");
    }

    /**
     *  aggregatorCommit implemented in PCE Service. TCERuntimeTCERuntime only handles replies.
     */
    public void aggregatorCreateCommit(AggregatorCreateCommitContent aggregatorCreateCommit )  {

        String event = PCERequestTypes.AGGREGATOR_CREATE_COMMIT;
        OSCARSNetLogger netLogger = OSCARSNetLogger.getTlogger();
        String transId = aggregatorCreateCommit.getMessageProperties().getGlobalTransactionId();
        netLogger.init("TCERuntime", transId);
        LOG.error(netLogger.error(event,ErrSev.MINOR,"Executing operation aggregatorCreateCommit - Should not happen"));
        // The PCE Runtime is not expected to receive any queries. Only replies.
        throw new RuntimeException("PCE Runtime received a aggregatorCreateCommit.");
    }

    /**
     *  aggregatorModifyCommit implemented in PCE Service. TCERuntime only handles replies.
     */
     public void aggregatorModifyCommit(AggregatorModifyCommitContent aggregatorCommit )  {

         String event = "aggregatorCommit";
         OSCARSNetLogger netLogger = OSCARSNetLogger.getTlogger();
         String transId = aggregatorCommit.getMessageProperties().getGlobalTransactionId();
        netLogger.init("TCERuntime", transId);
         LOG.error(netLogger.error(event,ErrSev.MINOR,"Executing operation aggregatorModifyCommit - Should not happen"));
         // The PCE Runtime is not expected to receive any queries. Only replies.
         throw new RuntimeException("PCE Runtime received a aggregatorModifyCommit.");
     }     

    /**
     *  aggregatorCancel implemented in PCE Service. TCERuntime only handles replies.
     */    
    public void aggregatorCancel(AggregatorCancelContent aggregatorCancel)  { 

        String event = PCERequestTypes.AGGREGATOR_CANCEL;
        OSCARSNetLogger netLogger = OSCARSNetLogger.getTlogger();
        String transId = aggregatorCancel.getMessageProperties().getGlobalTransactionId();
        netLogger.init("TCERuntime", transId);
        LOG.error(netLogger.error(event,ErrSev.MINOR,"Executing operation aggregatorCancel - Should not happen"));
        // The PCE Runtime is not expected to receive any queries. Only replies.
        throw new RuntimeException("PCE Runtime received a aggregatorCancel.");
    }
   
    /**
     *  pceCreateCommit implemented in PCE Service. TCERuntime only handles replies.
     */ 
    public void pceCreateCommit(PCECreateCommitContent pceCreateCommit ) {
        String event = PCERequestTypes.PCE_CREATE_COMMIT;
        OSCARSNetLogger netLogger = OSCARSNetLogger.getTlogger();
        String transId = pceCreateCommit.getMessageProperties().getGlobalTransactionId();
        netLogger.init("TCERuntime", transId);
        netLogger.setGRI(pceCreateCommit.getGlobalReservationId());
        LOG.error(netLogger.error(event,ErrSev.MINOR,"Executing operation pceCreateCommit - Should not happen"));
        // The PCE Runtime is not expected to receive any queries. Only replies.
        throw  new RuntimeException("Executing operation pceCreateCommit - Should not happen");
    }

    /**
     *  pceModifyCommit implemented in PCE Service. TCERuntime only handles replies.
     */ 
    public void pceModifyCommit(PCEModifyCommitContent pceModifyCommit ) {
        String event = PCERequestTypes.PCE_MODIFY_COMMIT;
        OSCARSNetLogger netLogger = OSCARSNetLogger.getTlogger();
        String transId = pceModifyCommit.getMessageProperties().getGlobalTransactionId();
        netLogger.init("TCERuntime", transId);
        netLogger.setGRI(pceModifyCommit.getGlobalReservationId());
        LOG.error(netLogger.error(event,ErrSev.MINOR,"Executing operation pceModifyCommit - Should not happen"));
        // The PCE Runtime is not expected to receive any queries. Only replies.
        throw  new RuntimeException("Executing operation pceModifyCommit - Should not happen");
    }

    /**
     *  pceModify implemented in PCE Service. TCERuntime only handles replies.
     */ 
    public void pceModify(PCEModifyContent pceModify) { 
        
        String method = "TCERuntimeSoapHandler.pceModify";
        OSCARSNetLogger netLogger = OSCARSNetLogger.getTlogger();
        String transId = pceModify.getMessageProperties().getGlobalTransactionId();
        netLogger.init("TCERuntime", transId);
        String globalReservationId = pceModify.getGlobalReservationId();
        LOG.error(netLogger.error(method, ErrSev.MINOR, "Executing operation pceModify - Should not happen"));
        // The PCE Runtime is not expected to receive any PCE Modify. Only replies.
        throw new RuntimeException("PCE Runtime received a PCEModify.");
    }
    
    /**
     *  pceCancel implemented in PCE Service. TCERuntime only handles replies.
     */ 
    public void pceCancel(PCECancelContent pceCancel) { 
        String method = "TCERuntimeSoapHandler.pceCancel";
        OSCARSNetLogger netLogger = OSCARSNetLogger.getTlogger();
        String transId = pceCancel.getMessageProperties().getGlobalTransactionId();
        netLogger.init("TCERuntime", transId);
        String globalReservationId = pceCancel.getGlobalReservationId();
        netLogger.setGRI(globalReservationId);
        LOG.error(netLogger.error(method, ErrSev.MINOR,"Executing operation pceCancel - Should not happen"));
        // The PCE Runtime is not expected to receive any PCE Cancel. Only replies.
        throw new RuntimeException("PCE Runtime received a PCECancel.");
    }

}
