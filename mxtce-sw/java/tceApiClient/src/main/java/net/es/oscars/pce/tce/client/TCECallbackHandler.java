/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package net.es.oscars.pce.tce.client;

import net.es.oscars.pce.soap.gen.v06.*;
    
/**
 *
 * @author xyang
 */
public class TCECallbackHandler {
    public static final Object lock = Object.class;

    synchronized public void handleReply(String method, String globalReservationId, 
            String transactionId, PCEDataContent pceDataContent, PCEError pceError, 
            String errorCode) throws RuntimeException  {
          throw new RuntimeException("Calling handleReply in TCECallbackHandler base class");
    }
}
