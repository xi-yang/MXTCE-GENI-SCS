/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package net.es.oscars.pce.tce.client.test;

import net.es.oscars.pce.tce.client.*;
import net.es.oscars.pce.soap.gen.v06.*;

import java.io.*;

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
               && pceDataContent.getOptionalConstraint().get(0).getValue().getStringValue() != null
               && !pceDataContent.getOptionalConstraint().get(0).getValue().getStringValue().isEmpty()) {
            String optionalConstraint = pceDataContent.getOptionalConstraint().get(0).getValue().getStringValue();
            System.out.println( "PCE optionalConstraint = "+optionalConstraint);
            try {
                FileWriter fstream = new FileWriter("mxtcePceReply_OC.xml");
                BufferedWriter out = new BufferedWriter(fstream);
                out.write(optionalConstraint);
                out.flush();
                out.close();
            } catch (Exception e) {
                System.err.println("Fail to write to mxtcePceReply.xml: " + e.getMessage());
            }
        }

        // retrieve error message
        if (pceDataContent == null && pceError != null) {
            System.err.println( "PCEErrorMsg="+pceError.getMsg());
        }
        // Do not exit in real app
        System.exit(0);
    }
}
