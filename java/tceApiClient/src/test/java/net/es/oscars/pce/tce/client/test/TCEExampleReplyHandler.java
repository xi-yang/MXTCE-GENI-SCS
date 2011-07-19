/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package net.es.oscars.pce.tce.client.test;

import net.es.oscars.pce.tce.client.*;
import net.es.oscars.pce.soap.gen.v06.*;

import org.w3c.dom.Element;

/**
 *
 * @author xyang
 */

public class TCEExampleReplyHandler extends TCECallbackHandler {
    @Override
    synchronized public void handleReply(String method, String globalReservationId, 
            String transactionId, PCEDataContent pceDataContent, PCEError pceError, 
            String errorCode) throws RuntimeException  {
        
        // retrieve basic info
        System.out.println("Method="+ method+" GRI="+globalReservationId
                + " transId=" + transactionId);
        
        // retrieve optionalConstraint as String
        if (pceDataContent != null && pceDataContent.getOptionalConstraint() != null 
               && !pceDataContent.getOptionalConstraint().isEmpty()
               && !pceDataContent.getOptionalConstraint().get(0).getValue().getAny().isEmpty()) {
            Element xmlElem = (Element)pceDataContent.getOptionalConstraint().get(0).getValue().getAny().get(0);
            String optionalConstraint = xmlElem.getTextContent();
            System.out.println( "PCEErroptionalConstraint = "+optionalConstraint);
        }

        // retrieve error message
        if (pceDataContent == null && pceError != null) {
            System.out.println( "PCEErrorMsg="+pceError.getMsg());
        }
        // Do not exit in real app
        System.exit(0);
    }
}
