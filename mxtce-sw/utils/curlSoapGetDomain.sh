#!/bin/sh

if [[ $1 == "all" || $1 == "ALL" ]]; then
cat > /tmp/ts.get.soap.curl.xml <<EOF
<?xml version='1.0' encoding='UTF-8'?>
<SOAP-ENV:Envelope xmlns:SOAP-ENC="http://schemas.xmlsoap.org/soap/encoding/"
     xmlns:xsd="http://www.w3.org/2001/XMLSchema"
     xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
     xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope">
    <SOAP-ENV:Header />
    <SOAP-ENV:Body>
       <nmwg:message type="QueryRequest" xmlns:nmwg="http://ggf.org/ns/nmwg/base/2.0/">
          <nmwg:metadata id="meta1">
             <nmwg:eventType>http://ggf.org/ns/nmwg/topology/20070809</nmwg:eventType>
          </nmwg:metadata>
          <nmwg:data id="data1" metadataIdRef="meta1" />
       </nmwg:message>
    </SOAP-ENV:Body>
</SOAP-ENV:Envelope>
EOF
else
cat > /tmp/ts.get.soap.curl.xml <<EOF
<?xml version='1.0' encoding='UTF-8'?>
<SOAP-ENV:Envelope xmlns:SOAP-ENC="http://schemas.xmlsoap.org/soap/encoding/"
     xmlns:xsd="http://www.w3.org/2001/XMLSchema"
     xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
     xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope">
    <SOAP-ENV:Header />
    <SOAP-ENV:Body>
       <nmwg:message type="QueryRequest" xmlns:nmwg="http://ggf.org/ns/nmwg/base/2.0/">
          <nmwg:metadata id="meta1">
             <xquery:subject id="sub1" xmlns:xquery="http://ggf.org/ns/nmwg/tools/org/perfsonar/xquery/1.0/">
                 //*[@id="urn:ogf:network:domain=$1"]
             </xquery:subject>
            <nmwg:eventType>http://ggf.org/ns/nmwg/topology/20070809</nmwg:eventType>
          </nmwg:metadata>
          <nmwg:data id="data1" metadataIdRef="meta1" />
       </nmwg:message>
    </SOAP-ENV:Body>
</SOAP-ENV:Envelope>
EOF
fi

TS_URL="http://dcn-ts.internet2.edu:8012/perfSONAR_PS/services/topology"
if [ $2 ]; then
    TS_URL=$2
fi

curl -H "Content-Type: text/xml; charset=utf-8" -d@/tmp/ts.get.soap.curl.xml $TS_URL
