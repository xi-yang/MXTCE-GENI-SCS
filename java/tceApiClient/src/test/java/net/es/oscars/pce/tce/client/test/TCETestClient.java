/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package net.es.oscars.pce.tce.client.test;

import java.net.URL;

import net.es.oscars.api.soap.gen.v06.*;

import net.es.oscars.pce.tce.client.*;

/**
 *
 * @author xyang
 */

/**
 * 
 * To use the TCE API client:
 *  1. Set $OSCARS_HOME to your base configuration directory
 *  2. Copy PCERuntimeService directory to under $OSCARS_HOME
 *  3. In your code, host URL to "http://tcePCE-server-name-or-IP:9020/tcePCE"
 *  4. Handle reply data in derived TCECallbackHandler::handleReply
 * 
 */


public class TCETestClient {
    public static void main(String args[]) {
        try {
            String host = "localhost";
            String port = "9020";
            if (args.length == 1) {
                host = args[0];
            }
            if (args.length == 2) {
                host = args[0];
                port = args[1];
            }
            URL hostUrl = new URL("http://"+host+":"+port+"/tcePCE");
            URL wsdlUrl = new URL("file://"+System.getenv("OSCARS_HOME")+"/PCERuntimeService/api/pce-0.6.wsdl");
            TCEApiClient apiClient = TCEApiClient.getClient(hostUrl, wsdlUrl, "MyTCEClient");
            TCEExampleReplyHandler replyHandler = new TCEExampleReplyHandler();
            apiClient.initClient(replyHandler);
            String srcUrn = "urn:ogf:network:domain=es.net:node=bnl-mr3:port=xe-1/2/0:link=*";
            String dstUrn = "urn:ogf:network:domain=ion.internet2.edu:node=rtr.chic:port=xe-6/0/0:link=*";
            long startTimeFromNow = 1317427200-1317234004;
            long duration = 100000;
            int bandwidth = 100; // in Mbps
            String vlan = "any"; 
            String descr = "my test path computation run";
            String optionalConstraint = "<coScheduleRequest id=\"schedule-124423245-option-1\">"
                + "<startTime>1317427200</startTime>"
                + "<endTime> 1317527200</endTime>"
                + "<minBandwidth>100</minBandwidth>"
                + "<maxNumOfAltPaths>3</maxNumOfAltPaths>"
                + "<bandwidthAvailabilityGraph>true</bandwidthAvailabilityGraph>"
                + "<contiguousVlan>true</contiguousVlan>"
                + "</coScheduleRequest>";
            //TODO: Debug optConstraint rootElem ?
            ResCreateContent reqData = apiClient.assembleResCreateContent( srcUrn, dstUrn, 
            startTimeFromNow, duration, bandwidth, vlan, descr, optionalConstraint, null);
            apiClient.sendPceCreate(reqData);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
