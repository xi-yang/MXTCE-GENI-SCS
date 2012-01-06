/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package net.es.oscars.pce.tce.client.test;

import java.net.URL;

import net.es.oscars.api.soap.gen.v06.*;

import net.es.oscars.pce.tce.client.*;

import java.util.*;


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
            String yamlfile="";
            String host = "localhost";
            String port = "9020";
            if (args.length == 1) {
                yamlfile = args[0];
            }
            if (args.length == 2) {
                yamlfile = args[0];
                host = args[1];
            }
            if (args.length == 3) {
                yamlfile = args[0];
                host = args[1];
                port = args[2];
            }
            URL hostUrl = new URL("http://"+host+":"+port+"/tcePCE");
            URL wsdlUrl = new URL("file://"+System.getenv("OSCARS_HOME")+"/PCERuntimeService/api/pce-0.6.wsdl");
            TCEApiClient apiClient = TCEApiClient.getClient(hostUrl, wsdlUrl, "MyTCEClient");
            TCEExampleReplyHandler replyHandler = new TCEExampleReplyHandler();
            apiClient.initClient(replyHandler);
            String srcUrn = "urn:ogf:network:domain=testdomain-1.net:node=node-1:port=port-1:link=link-1";
            String dstUrn = "urn:ogf:network:domain=testdomain-1.net:node=node-3:port=port-4:link=link-1";
            String vlan = "any"; 
            String descr = "my test path computation run";
            String optionalConstraint = "<coScheduleRequest id=\"schedule-123456789-option-1\">"
                + "<startTime>_starttime_</startTime>"
                + "<endTime>_endtime_</endTime>"
                + "<minBandwidth>_bandwidth_</minBandwidth>"
                + "<maxNumOfAltPaths>3</maxNumOfAltPaths>"
                + "<bandwidthAvailabilityGraph>true</bandwidthAvailabilityGraph>"
                + "<contiguousVlan>true</contiguousVlan>"
                + "</coScheduleRequest>";
            ResCreateContent reqData;
            if (yamlfile.isEmpty()) {
                HashMap<String, Long> times = TCEApiClient.parseTimes("now", "+00:00:30");
                optionalConstraint = optionalConstraint.replaceAll("_starttime_", Long.toString(times.get("start")));
                optionalConstraint = optionalConstraint.replaceAll("_endtime_", Long.toString(times.get("end")));
                optionalConstraint = optionalConstraint.replaceAll("_bandwidth_", Integer.toString(100));
                reqData = apiClient.assembleResCreateContent( srcUrn, dstUrn, 0, 1800, 100, vlan, descr, optionalConstraint, null);
            } else {
                reqData = apiClient.configureYaml(yamlfile, optionalConstraint);
            }
            apiClient.sendPceCreate(reqData);                
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
 