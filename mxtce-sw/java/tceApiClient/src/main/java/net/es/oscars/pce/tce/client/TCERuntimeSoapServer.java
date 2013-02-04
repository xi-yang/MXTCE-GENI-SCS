/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package net.es.oscars.pce.tce.client;


import net.es.oscars.utils.soap.OSCARSService;
import net.es.oscars.utils.soap.OSCARSSoapService;
import net.es.oscars.utils.soap.OSCARSServiceException;
import net.es.oscars.utils.config.ConfigDefaults;
import net.es.oscars.logging.OSCARSNetLoggerize;
import net.es.oscars.pce.soap.gen.v06.PCEPortType;
import net.es.oscars.pce.soap.gen.v06.PCEService;
import net.es.oscars.utils.config.ContextConfig;
import net.es.oscars.utils.config.SharedConfig;

import java.io.File;

/**
 *
 * @author xyang
 */
@OSCARSNetLoggerize(moduleName="PCERuntime")
@OSCARSService (
		implementor = "net.es.oscars.pce.tce.client.TCERuntimeSoapHandler",
		serviceName = "PCERuntimeService",
		config = "config.yaml"
)
public class TCERuntimeSoapServer extends OSCARSSoapService  <PCEService, PCEPortType> {
 
    private static TCERuntimeSoapServer instance = null;

    private TCERuntimeSoapServer() throws OSCARSServiceException {
        // Uses the default ContextConfig
        super("PCERuntimeService");        
        System.out.println("service config"+this.getConfig().toString());
    }
    
    public static TCERuntimeSoapServer getInstance() throws OSCARSServiceException {
        ContextConfig cc = ContextConfig.getInstance("PCERuntimeService");
        if (instance == null) {
            instance = new TCERuntimeSoapServer();
        }
        return instance;
    }
    
    public void setCallbackHandler(TCECallbackHandler handler) throws OSCARSServiceException {
        try {
            TCERuntimeSoapHandler impl = (TCERuntimeSoapHandler)getInstance().getPortType();
            impl.setCallbackHandler(handler);
        } catch (Exception e) {
            e.printStackTrace();
            throw new OSCARSServiceException (e.toString());
        }

    }
    
    public TCECallbackHandler getCallbackHandler() {
        try {
            TCERuntimeSoapHandler impl = (TCERuntimeSoapHandler)getInstance().getPortType();
            return impl.getCallbackHandler();
        } catch (Exception e) {
            return null;
        }
    }
}

