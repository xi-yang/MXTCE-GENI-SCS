/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package net.es.oscars.pce.tce.client.test;

import java.net.URL;

import net.es.oscars.api.soap.gen.v06.*;

import net.es.oscars.pce.tce.client.*;

import java.io.*;

import java.util.*;

import java.text.SimpleDateFormat;

import org.ho.yaml.Yaml;

import org.ogf.schema.network.topology.ctrlplane.*;

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
                HashMap<String, Long> times = parseTimes("now", "+00:00:30");
                optionalConstraint = optionalConstraint.replaceAll("_starttime_", Long.toString(times.get("start")));
                optionalConstraint = optionalConstraint.replaceAll("_endtime_", Long.toString(times.get("end")));
                optionalConstraint = optionalConstraint.replaceAll("_bandwidth_", Integer.toString(100));
                reqData = apiClient.assembleResCreateContent( srcUrn, dstUrn, 0, 1800, 100, vlan, descr, optionalConstraint, null);
            } else {
                reqData = configureYaml(yamlfile, optionalConstraint);
            }
            apiClient.sendPceCreate(reqData);                
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @SuppressWarnings("unchecked")
    public static ResCreateContent configureYaml(String configFile, String optionalConstraint) {
        Map config = getConfiguration(configFile);
        Map store = (Map) config.get("create");

        ResCreateContent resContent = new ResCreateContent();
        String gri = (String) store.get("gri");
        String login = (String) store.get("login");
        String layer = (String)store.get("layer");
        Integer bandwidth = (Integer) store.get("bandwidth");
        Integer burstLimit = (Integer) store.get("burstLimit");
        String lspClass = (String) store.get("lspClass");
        String src = (String) store.get("src");
        String dst = (String) store.get("dst");
        String description = (String) store.get("description");
        String srcVlan = (String) store.get("srcvlan");
        String dstVlan = (String) store.get("dstvlan");
        String start_time = (String) store.get("start-time");
        String end_time = (String) store.get("end-time");
        ArrayList<String> pathArray = (ArrayList<String>) store.get("path");
        String pathSetupMode = (String) store.get("path-setup-mode");
        String pathType = (String) store.get("path-type");
        ArrayList<Map> optConArray = (ArrayList<Map>) store.get("optionalConstraint");

        if (!layer.equals("2") && !layer.equals("3")) {
            die("Layer must be 2 or 3");
        }
        if (src == null || src.equals("")) {
            die("Source must be specified");
        }
        if (dst == null || dst.equals("")) {
            die("Destination must be specified");
        }
        if (bandwidth == null) {
            die("bandwidth must be specified");
        }
        if (description == null || description.equals("")) {
            die("description must be specified");
        }
        HashMap<String, Long> times = parseTimes(start_time, end_time);

        //if (gri != null && gri.length() != 0) {
        if (gri != null ) { 
            resContent.setGlobalReservationId(gri);
        }
        resContent.setDescription(description);
        UserRequestConstraintType uc = new UserRequestConstraintType();
        uc.setBandwidth(bandwidth);
        uc.setStartTime(times.get("start"));
        uc.setEndTime(times.get("end"));
        PathInfo pathInfo =  new PathInfo();
        pathInfo.setPathSetupMode(pathSetupMode);
        pathInfo.setPathType(pathType);
        if (layer.equals("2")) {
            Layer2Info layer2Info = new Layer2Info();
            layer2Info.setSrcEndpoint(src);
            layer2Info.setDestEndpoint(dst);
            if (srcVlan != null) {
                VlanTag vlan = new VlanTag();
                vlan.setValue(srcVlan);
                vlan.setTagged(true);
                layer2Info.setSrcVtag(vlan);
            }
            if (dstVlan != null) {
                VlanTag vlan = new VlanTag();
                vlan.setValue(dstVlan);
                vlan.setTagged(true);
                layer2Info.setDestVtag(vlan);
            }
            pathInfo.setLayer2Info(layer2Info);
        } else if (layer.equals("3")) {
            // TODO
            if (burstLimit !=  null && burstLimit != 0){
                MplsInfo mplsInfo = new MplsInfo();
                mplsInfo.setBurstLimit(burstLimit);
                mplsInfo.setLspClass(lspClass);
                pathInfo.setMplsInfo(mplsInfo);
            }
        }
        if ( pathArray != null && !pathArray.isEmpty() ) {
            CtrlPlanePathContent path = new CtrlPlanePathContent ();
            List<CtrlPlaneHopContent> hops = path.getHop();
            for (String hop : pathArray) {
               CtrlPlaneHopContent cpHop = new CtrlPlaneHopContent();
               cpHop.setLinkIdRef(hop);
               hops.add(cpHop);
            }
            pathInfo.setPath(path);
        }

        if (optConArray != null && !optConArray.isEmpty()) {
            for (Map optCon : optConArray) {
                OptionalConstraintType oc = new OptionalConstraintType();
                OptionalConstraintValue ocv = new OptionalConstraintValue();
                oc.setCategory((String)optCon.get("category"));
                System.out.println("oc category is " + optCon.get("category"));
                String value = (String) optCon.get("value");
                System.out.println("oc value is " + optCon.get("value"));
                ocv.setStringValue(value);
                oc.setValue(ocv);
                resContent.getOptionalConstraint().add(oc);
            }
        }
        uc.setPathInfo(pathInfo);
        resContent.setUserRequestConstraint(uc);

        //set optional constraint
        if (optionalConstraint != null && optionalConstraint.length() > 0) {
            optionalConstraint = optionalConstraint.replaceAll("_starttime_", Long.toString(times.get("start")));
            optionalConstraint = optionalConstraint.replaceAll("_endtime_", Long.toString(times.get("end")));
            optionalConstraint = optionalConstraint.replaceAll("_bandwidth_", Integer.toString(bandwidth));
            OptionalConstraintValue optValue = new OptionalConstraintValue();
            optValue.setStringValue(optionalConstraint);
            OptionalConstraintType optType = new OptionalConstraintType();
            optType.setValue(optValue);
            optType.setCategory("api-experiment-stornet");
            resContent.getOptionalConstraint().add(optType);
        }
        return resContent;
    }

    
    @SuppressWarnings({ "static-access", "unchecked" })
    static public Map getConfiguration(String filename) {
    	Map configuration = null;
    	
        if (configuration == null) {
            InputStream propFile = TCETestClient.class.getClassLoader().getSystemResourceAsStream(filename);
            try {
                configuration = (Map) Yaml.load(propFile);
            } catch (NullPointerException ex) {
                try {
                    propFile = new FileInputStream(new File(filename));
                } catch (FileNotFoundException e) {
                    System.out.println("TCETestClient: configuration file: "+ filename + " not found");
                    e.printStackTrace();
                    System.exit(1);
                }
                configuration = (Map) Yaml.load(propFile);
            }
        }
        return configuration;
    }

    public static Long parseTime(String time){
       SimpleDateFormat df = new SimpleDateFormat("yyyy-MM-dd:HH:mm");
        Long outTime = 0L;
        try {
            outTime = df.parse(time.trim()).getTime()/1000;
        } catch (java.text.ParseException ex) {
                die("Error parsing start date: "+ex.getMessage());
        }
        return outTime;
    }

    private static HashMap<String, Long> parseTimes(String start_time, String end_time) {
        HashMap<String, Long> result = new HashMap<String, Long>();
        Long startTime = 0L;
        Long endTime = 0L;
        Long createTime= System.currentTimeMillis()/1000;
        SimpleDateFormat df = new SimpleDateFormat("yyyy-MM-dd HH:mm");
        if (start_time == null || start_time.equals("now") || start_time.equals("")) {
            startTime = System.currentTimeMillis()/1000;
        } else {
            try {
                startTime = df.parse(start_time.trim()).getTime()/1000;
            } catch (java.text.ParseException ex) {
                die("Error parsing start date: "+ex.getMessage());
            }
        }
        if (end_time == null || end_time.equals("")) {
            die("No end time specified.");
        } else if (end_time.startsWith("+")) {
            String[] hm = end_time.substring(1).split("\\:");
            if (hm.length != 3) {
                die("Error parsing end date format");
            } 
            try {
                Integer seconds = Integer.valueOf(hm[0])*3600*24; //days
                seconds += Integer.valueOf(hm[1])*3600; // hours
                seconds += Integer.valueOf(hm[2])*60; // minutes
                if (seconds < 60) {
                    die("Duration must be > 60 sec");
                }
                endTime = startTime + seconds;
            } catch (NumberFormatException ex) {
                die("Error parsing end date format: "+ex.getMessage());
            }
        } else {
            try {
                endTime = df.parse(end_time.trim()).getTime()/1000;
            } catch (java.text.ParseException ex) {
                die("Error parsing emd date: "+ex.getMessage());
            }
        }
        
        
        result.put("start", startTime);
        result.put("end", endTime);
        result.put("create", createTime);
        return result;
    }

    private static void die(String msg) {
        System.err.println(msg);
        System.exit(1);
    }
}
