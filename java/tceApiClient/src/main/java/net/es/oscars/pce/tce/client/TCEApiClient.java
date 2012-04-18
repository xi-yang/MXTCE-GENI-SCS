/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package net.es.oscars.pce.tce.client;

/**
 *
 * @author xyang
 */

import org.apache.log4j.Logger;
import java.net.URL;
import java.net.MalformedURLException;

import org.apache.cxf.jaxws.EndpointImpl;

import net.es.oscars.utils.config.ConfigDefaults;
import net.es.oscars.utils.config.ConfigException;
import net.es.oscars.utils.config.ContextConfig;

import net.es.oscars.utils.topology.NMWGTopoBuilder;
import net.es.oscars.logging.*;
import net.es.oscars.api.soap.gen.v06.*;
import net.es.oscars.pce.soap.gen.v06.*;
import net.es.oscars.common.soap.gen.MessagePropertiesType;
import net.es.oscars.common.soap.gen.SubjectAttributes;
import oasis.names.tc.saml._2_0.assertion.AttributeType;

import net.es.oscars.utils.soap.OSCARSServiceException;
import net.es.oscars.utils.soap.OSCARSService;
import net.es.oscars.utils.soap.OSCARSSoapService;

import net.es.oscars.logging.OSCARSNetLoggerize;
import net.es.oscars.logging.OSCARSNetLogger;

import javax.xml.bind.JAXBContext;
import javax.xml.bind.Unmarshaller;
import javax.xml.bind.JAXBElement;

import java.io.*;
import java.util.*;

import java.text.SimpleDateFormat;

import org.ho.yaml.Yaml;
import org.ogf.schema.network.topology.ctrlplane.*;

@OSCARSNetLoggerize(moduleName="TCEApi")
@OSCARSService (
        implementor = "net.es.oscars.pce.soap.gen.v06.PCEService",
        namespace   = "http://oscars.es.net/OSCARS/PCE/20090922",
        serviceName = "PCEService",
        config      = ConfigDefaults.CONFIG
)
public class TCEApiClient extends OSCARSSoapService<PCEService, PCEPortType> {

    private static final Logger LOG = Logger.getLogger(TCEApiClient.class.getName());

    // TODO: using config file
    private static final String PceName = "tcePCE";
    private static String CallBackEndpoint = "http://127.0.0.1:10000/OSCARS/PCERuntimeService";
    private static int griAutoNum = 1;

    private static TCERuntimeSoapServer runtimeServer = null;
    private static EndpointImpl runtimeEndpoint = null;

    private String clientName = "TestTCEClient";
    

    
    private TCEApiClient (URL host, URL wsdlFile, String clientName) throws OSCARSServiceException {
    	super (host, wsdlFile, PCEPortType.class);
    }
    
    static public TCEApiClient getClient (URL host, URL wsdl, String clientName) 
            throws MalformedURLException, OSCARSServiceException {
        ContextConfig cc = ContextConfig.getInstance();
        String cxfClientPath = System.getenv("OSCARS_HOME")+"/PCERuntimeService/conf/client-cxf-http.xml";
        System.out.println("TCEApiClient setting BusConfiguration from " + cxfClientPath);
        OSCARSSoapService.setSSLBusConfiguration(new URL("file:" + cxfClientPath));
        TCEApiClient client = new TCEApiClient (host, wsdl, clientName);
        return client;
    }

    public void initClient(TCECallbackHandler replyHandler) throws OSCARSServiceException {
        if (runtimeServer == null)
            runtimeServer = TCERuntimeSoapServer.getInstance();
        runtimeServer.setCallbackHandler(replyHandler);
        runtimeEndpoint = (EndpointImpl)runtimeServer.startServer(false);
        Map soap = (Map)runtimeServer.getConfig().get("soap");
        if (soap != null && soap.get("publishTo") != null)
            CallBackEndpoint = (String)soap.get("publishTo");
    }

    public void stopClient() {
        if (runtimeEndpoint != null)
            runtimeEndpoint.stop();
    }

    public TCECallbackHandler getReplyHandler() {
        return runtimeServer.getCallbackHandler();
    }

    private String getAutoGRI() {
        return clientName+"-"+Integer.toString(griAutoNum++);
    }

    private String getTransactionId() {
        return clientName+"-"+UUID.randomUUID().toString();
    }

    public void sendPceCreate (String gri, PCEDataContent pceDataContent) 
            throws OSCARSServiceException {
        if (gri == null || gri.isEmpty())
            gri = getAutoGRI();
 
        OSCARSNetLogger netLogger = OSCARSNetLogger.getTlogger();
        String event = "sendPCECreate";
        LOG.info (netLogger.start(event));
        
        // Build the PCECreate request
        String transId = this.getTransactionId();
        PCECreateContent queryContent = new PCECreateContent();
        SubjectAttributes subjectAttrs = new SubjectAttributes();
        List<AttributeType> reqAttrs = subjectAttrs.getSubjectAttribute();
        AttributeType attr1 = new AttributeType();
        attr1.setName("loginId");
        attr1.getAttributeValue().add("client");
        reqAttrs.add(attr1);
        MessagePropertiesType msgProps = new MessagePropertiesType();
        msgProps.setOriginator(subjectAttrs);
        msgProps.setGlobalTransactionId(transId);
        queryContent.setMessageProperties(msgProps);
        queryContent.setGlobalReservationId(gri);
        queryContent.setPceName(PceName);
        queryContent.setCallBackEndpoint(CallBackEndpoint);
        queryContent.setId(transId);
        queryContent.setPceData(pceDataContent);
        Object[] req = new Object[] {queryContent};
        this.invoke("PCECreate", req);
        LOG.info (netLogger.end(event));
    }

    // saved requestContent and updated optionalConstraint
    public void sendPceCreateCommit (String gri, PCEDataContent pceDataContent, 
            String optionalConstraint) throws OSCARSServiceException {

        OSCARSNetLogger netLogger = OSCARSNetLogger.getTlogger();
        String event = "sendPCECreateCommit";
        LOG.info (netLogger.start(event));
        
        if (gri == null || gri.isEmpty())
            throw new OSCARSServiceException("sendPceCreateCommit uses null or empty GRI");

        // Build the PCECreateCommit request
        String transId = this.getTransactionId();
        pceDataContent.setTopology(makeTopology(pceDataContent.getReservedConstraint()));
        if (pceDataContent.getOptionalConstraint() != null && optionalConstraint != null) {
            OptionalConstraintValue optValue = new OptionalConstraintValue();
            optValue.setStringValue(optionalConstraint);
            OptionalConstraintType optType = new OptionalConstraintType();
            optType.setValue(optValue);
            optType.setCategory("api-experiment-stornet");
            pceDataContent.getOptionalConstraint().clear();
            pceDataContent.getOptionalConstraint().add(optType);
        }
        
        List<TagDataContent> tagDataContent = null;

        PCECreateCommitContent queryContent = new PCECreateCommitContent();
        SubjectAttributes subjectAttrs = new SubjectAttributes();
        List<AttributeType> reqAttrs = subjectAttrs.getSubjectAttribute();
        AttributeType attr1 = new AttributeType();
        attr1.setName("loginId");
        attr1.getAttributeValue().add("client");
        reqAttrs.add(attr1);
        MessagePropertiesType msgProps = new MessagePropertiesType();
        msgProps.setOriginator(subjectAttrs);
        msgProps.setGlobalTransactionId(transId);
        queryContent.setMessageProperties(msgProps);
        queryContent.setGlobalReservationId(gri);
        queryContent.setPceName(PceName);
        queryContent.setCallBackEndpoint(CallBackEndpoint);
        queryContent.setId(transId);
        queryContent.setPceData(pceDataContent);
        Object[] req = new Object[] {queryContent};
        this.invoke("PCECreateCommit", req);
        LOG.info (netLogger.end(event));
    }
    
    
    public void sendPceCancel (String gri, PCEDataContent pceDataContent, 
            String optionalConstraint) throws OSCARSServiceException {
 
        OSCARSNetLogger netLogger = OSCARSNetLogger.getTlogger();
        String event = "sendPCECancel";
        LOG.info (netLogger.start(event));
        if (gri == null || gri.isEmpty())
            throw new OSCARSServiceException("sendPceCancel uses null or empty GRI");

        // Build the PCECancel request
        String transId = this.getTransactionId();
        pceDataContent.setTopology(makeTopology(pceDataContent.getReservedConstraint()));
        if (pceDataContent.getOptionalConstraint() != null && optionalConstraint != null) {
            OptionalConstraintValue optValue = new OptionalConstraintValue();
            optValue.setStringValue(optionalConstraint);
            OptionalConstraintType optType = new OptionalConstraintType();
            optType.setValue(optValue);
            optType.setCategory("api-experiment-stornet");
            pceDataContent.getOptionalConstraint().clear();
            pceDataContent.getOptionalConstraint().add(optType);
        }
        PCECancelContent queryContent = new PCECancelContent();
        SubjectAttributes subjectAttrs = new SubjectAttributes();
        List<AttributeType> reqAttrs = subjectAttrs.getSubjectAttribute();
        AttributeType attr1 = new AttributeType();
        attr1.setName("loginId");
        attr1.getAttributeValue().add("client");
        reqAttrs.add(attr1);
        MessagePropertiesType msgProps = new MessagePropertiesType();
        msgProps.setOriginator(subjectAttrs);
        msgProps.setGlobalTransactionId(transId);
        queryContent.setMessageProperties(msgProps);
        queryContent.setGlobalReservationId(gri);
        queryContent.setPceName(PceName);
        queryContent.setCallBackEndpoint(CallBackEndpoint);
        queryContent.setId(transId);
        queryContent.setPceData(pceDataContent);
        Object[] req = new Object[] {queryContent};
        this.invoke("PCECancel", req);
        LOG.info (netLogger.end(event));
    }
 
    // standard p2p path request
    public PCEDataContent assemblePceData(String srcUrn, String dstUrn, 
            long startTimeFromNow, long duration, int bandwidth, String vlan, 
            String optionalConstraint) {
        UserRequestConstraintType userConstraint = new UserRequestConstraintType();
        long currentTime = System.currentTimeMillis() / 1000;
        String pathId = "path-"+ UUID.randomUUID().toString();
        userConstraint.setStartTime(currentTime + startTimeFromNow);
        userConstraint.setEndTime(currentTime + startTimeFromNow + duration);
        userConstraint.setBandwidth(bandwidth);
        Layer2Info layer2Info = new Layer2Info();
        layer2Info.setSrcEndpoint(srcUrn);
        layer2Info.setDestEndpoint(dstUrn);
        VlanTag srcVtag = new VlanTag();
        srcVtag.setValue(vlan);
        srcVtag.setTagged(true);
        VlanTag dstVtag = new VlanTag();
        dstVtag.setValue(vlan);
        dstVtag.setTagged(true);
        layer2Info.setSrcVtag(srcVtag);
        layer2Info.setDestVtag(dstVtag);
        PathInfo pathInfo = new PathInfo();
        pathInfo.setLayer2Info(layer2Info);
        pathInfo.setPathSetupMode("timer-automatic");
        pathInfo.setPathType("strict");
        CtrlPlanePathContent pathContent = new CtrlPlanePathContent();
        pathContent.setId(pathId);
        pathInfo.setPath(pathContent);
        userConstraint.setPathInfo(pathInfo);
        
        ReservedConstraintType reservedConstraint = new ReservedConstraintType();
        reservedConstraint.setStartTime(currentTime + startTimeFromNow);
        reservedConstraint.setEndTime(currentTime + startTimeFromNow + duration);
        reservedConstraint.setBandwidth(bandwidth);
        pathInfo = new PathInfo();
        pathInfo.setLayer2Info(layer2Info);
        pathContent = new CtrlPlanePathContent();
        pathContent.setId(pathId);
        CtrlPlaneHopContent srcHop = makeEdgeHop(srcUrn, vlan);
        CtrlPlaneHopContent dstHop = makeEdgeHop(dstUrn, vlan);
        pathContent.getHop().add(srcHop);
        pathContent.getHop().add(dstHop);
        pathInfo.setPath(pathContent);
        reservedConstraint.setPathInfo(pathInfo);        

        PCEDataContent pceDataContent = new PCEDataContent();
        pceDataContent.setUserRequestConstraint(userConstraint);
        pceDataContent.setReservedConstraint(reservedConstraint);
        if (optionalConstraint != null) {
            OptionalConstraintValue optValue = new OptionalConstraintValue();
            optValue.setStringValue(optionalConstraint);
            OptionalConstraintType optType = new OptionalConstraintType();
            optType.setValue(optValue);
            optType.setCategory("api-experiment-stornet");
            pceDataContent.getOptionalConstraint().add(optType);
        }

        return pceDataContent;
    }

    public static CtrlPlaneHopContent makeEdgeHop(String linkId, String vlan) {
        
        CtrlPlaneHopContent hop          = new CtrlPlaneHopContent();
        
        CtrlPlaneLinkContent link        = new CtrlPlaneLinkContent();
        CtrlPlaneSwcapContent scp        = new CtrlPlaneSwcapContent();
        CtrlPlaneSwitchingCapabilitySpecificInfo ssi
                                         = new CtrlPlaneSwitchingCapabilitySpecificInfo();
        
        hop.setLinkIdRef(linkId);
        link.setId(linkId);
        ssi.setSuggestedVLANRange(vlan);
        ssi.setVlanRangeAvailability(vlan);
        scp.setSwitchingCapabilitySpecificInfo(ssi);
        link.getSwitchingCapabilityDescriptors().add(scp);
        hop.setLink(link);
        return hop;
    }

    public static CtrlPlaneTopologyContent makeTopology(ReservedConstraintType reservedConstraint) 
            throws OSCARSServiceException  {
        CtrlPlaneTopologyContent topoContent = new CtrlPlaneTopologyContent();
        NMWGTopoBuilder topoBuilder = new NMWGTopoBuilder();
        if (reservedConstraint == null || reservedConstraint.getPathInfo() == null
               || reservedConstraint.getPathInfo().getPath() == null
               || reservedConstraint.getPathInfo().getPath().getHop().isEmpty()) {
            throw new OSCARSServiceException("makeTopology failed on empty or malformed reservedConstraint");
        }
        List<CtrlPlaneHopContent> hops = reservedConstraint.getPathInfo().getPath().getHop();
        for (CtrlPlaneHopContent hop : hops) {
            CtrlPlaneLinkContent link = hop.getLink();
            try {
                topoBuilder.addLink(link);
            } catch (OSCARSServiceException e) {
                e.printStackTrace();
                throw e;
            }
        }
        return topoBuilder.getTopology();
    }

    // assemble ARCHSTONE Request Topology
    public PCEDataContent assemblePceData(CtrlPlaneTopologyContent topology, 
            String optionalConstraint) {
        UserRequestConstraintType userConstraint = new UserRequestConstraintType();
        String pathId = "path-"+ UUID.randomUUID().toString();
        // startTime, endTime and bandwidth in userReqConstraint no longer matter
        //userConstraint.setStartTime(0);
        //userConstraint.setEndTime(0);
        //userConstraint.setBandwidth(0);
        PathInfo pathInfo = new PathInfo();
        // no more Layer2Info or Layer3Info at in pathInfo of userReqConstraint
        pathInfo.setPathType("RequestTopology");
        pathInfo.setPathSetupMode("timer-automatic");
        CtrlPlanePathContent pathContent = new CtrlPlanePathContent();
        pathContent.setId(pathId);
        pathInfo.setPath(pathContent);
        userConstraint.setPathInfo(pathInfo);

       
        PCEDataContent pceDataContent = new PCEDataContent();
        pceDataContent.setUserRequestConstraint(userConstraint);
        if (optionalConstraint != null) {
            OptionalConstraintValue optValue = new OptionalConstraintValue();
            optValue.setStringValue(optionalConstraint);
            OptionalConstraintType optType = new OptionalConstraintType();
            optType.setValue(optValue);
            optType.setCategory("api-experiment-stornet");
            pceDataContent.getOptionalConstraint().add(optType);
        }
        
        pceDataContent.setTopology(topology);
        return pceDataContent;
    }

    // assemble ARCHSTONE Request Topology from XML string
    public PCEDataContent assemblePceData(String reqTopologyXml, 
            String optionalConstraint) {
        //unmarshal handle <topology> XML with JAXB and add to pceData 
        CtrlPlaneTopologyContent topology = null;
        try {
            StringReader reader = new StringReader(reqTopologyXml);
            JAXBContext jc = JAXBContext.newInstance("org.ogf.schema.network.topology.ctrlplane");
            Unmarshaller unm = jc.createUnmarshaller();
            JAXBElement<CtrlPlaneTopologyContent> jaxbTopology = (JAXBElement<CtrlPlaneTopologyContent>) unm.unmarshal(reader);
            topology = jaxbTopology.getValue();
        } catch (Exception e) {
            System.err.println("Error in unmarshling RequestTopology: " + e.getMessage());
        }
        return assemblePceData(topology, optionalConstraint);
    }

    @SuppressWarnings("unchecked")
    public static ResCreateContent configureRequstFromYaml(String configFile, String optionalConstraint) {
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
        Layer2Info layer2Info = new Layer2Info();
        VlanTag vlan = new VlanTag();
        if (layer.equals("2")) {
            layer2Info.setSrcEndpoint(src);
            layer2Info.setDestEndpoint(dst);
            if (srcVlan != null) {
                vlan.setValue(srcVlan);
                vlan.setTagged(true);
                layer2Info.setSrcVtag(vlan);
            }
            if (dstVlan != null) {
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
        CtrlPlanePathContent path = new CtrlPlanePathContent ();
        if ( pathArray != null && !pathArray.isEmpty() ) {
            List<CtrlPlaneHopContent> hops = path.getHop();
            for (String hop : pathArray) {
               CtrlPlaneHopContent cpHop = new CtrlPlaneHopContent();
               cpHop.setLinkIdRef(hop);
               hops.add(cpHop);
            }
            pathInfo.setPath(path);
        }

        uc.setPathInfo(pathInfo);
        resContent.setUserRequestConstraint(uc);

        ReservedConstraintType reservedConstraint = new ReservedConstraintType();
        reservedConstraint.setBandwidth(bandwidth);
        pathInfo = new PathInfo();
        pathInfo.setLayer2Info(layer2Info);
        path = new CtrlPlanePathContent();
        String pathId = "path-"+ UUID.randomUUID().toString();
        path.setId(pathId);
        CtrlPlaneHopContent srcHop = makeEdgeHop(src, srcVlan);
        CtrlPlaneHopContent dstHop = makeEdgeHop(dst, dstVlan);
        path.getHop().add(srcHop);
        path.getHop().add(dstHop);
        pathInfo.setPath(path);
        reservedConstraint.setPathInfo(pathInfo);        
        resContent.setReservedConstraint(reservedConstraint);

        //set optional constraint
        if (optionalConstraint != null && optionalConstraint.length() > 0) {
            optionalConstraint = optionalConstraint.replaceAll("_starttime_", Long.toString(times.get("start")));
            optionalConstraint = optionalConstraint.replaceAll("_endtime_", Long.toString(times.get("end")));
            optionalConstraint = optionalConstraint.replaceAll("_bandwidth_", Long.toString((long)bandwidth*1000000));
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
            InputStream propFile = TCEApiClient.class.getClassLoader().getSystemResourceAsStream(filename);
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

    public static HashMap<String, Long> parseTimes(String start_time, String end_time) {
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
