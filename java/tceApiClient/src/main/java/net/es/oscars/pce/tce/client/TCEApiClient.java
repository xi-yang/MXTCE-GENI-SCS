/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package net.es.oscars.pce.tce.client;

/**
 *
 * @author xyang
 */

import java.util.List;
import java.util.Map;
import java.util.UUID;
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
import net.es.oscars.logging.ErrSev;
import net.es.oscars.logging.OSCARSNetLogger;

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

    private String getAutoGRI() {
        return clientName+"-"+Integer.toString(griAutoNum++);
    }

    private String getTransactionId() {
        return clientName+"-"+UUID.randomUUID().toString();
    }

    public void sendPceCreate (ResCreateContent requestContent) 
            throws OSCARSServiceException {
 
        OSCARSNetLogger netLogger = OSCARSNetLogger.getTlogger();
        String event = "sendPCECreate";
        LOG.info (netLogger.start(event));

        PCEDataContent pceDataContent = new PCEDataContent();
        pceDataContent.setUserRequestConstraint (requestContent.getUserRequestConstraint());
        pceDataContent.setReservedConstraint (requestContent.getReservedConstraint());
        if (requestContent.getOptionalConstraint() != null) {
            // Optional constraints may not be defined.
            pceDataContent.getOptionalConstraint().addAll(requestContent.getOptionalConstraint());
        }
        
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
        queryContent.setGlobalReservationId(requestContent.getGlobalReservationId());
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
 
    public ResCreateContent assembleResCreateContent(String srcUrn, String dstUrn, 
            long startTimeFromNow, long duration, int bandwidth, String vlan, 
            String descr, String optionalConstraint, String gri) {
        if (gri == null || gri.isEmpty())
            gri = getAutoGRI();
        ResCreateContent query = new ResCreateContent();
        query.setGlobalReservationId(gri);
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
        query.setDescription(descr);
        query.setUserRequestConstraint(userConstraint);
        
        ReservedConstraintType reservedConstraint = new ReservedConstraintType();
        userConstraint.setStartTime(currentTime + startTimeFromNow);
        userConstraint.setEndTime(currentTime + startTimeFromNow + duration);
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
        query.setReservedConstraint(reservedConstraint);
        OptionalConstraintValue optValue = new OptionalConstraintValue();
        optValue.setStringValue(optionalConstraint);
        OptionalConstraintType optType = new OptionalConstraintType();
        optType.setValue(optValue);
        optType.setCategory("api-experiment-stornet");
        query.getOptionalConstraint().add(optType);

        return query;
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
        link.setSwitchingCapabilityDescriptors(scp);
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
}
