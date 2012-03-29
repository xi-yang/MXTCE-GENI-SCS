#!/bin/sh
#usage test.sh 
if [ ! -f target/tceApiClient-0.0.1-SNAPSHOT.jar ]; then
    echo
    echo "!!! run 'mvn install' to compile the code"
    echo
    exit 1
fi
. bin/setclasspath.sh
OSCARS_HOME=`pwd`
java net.es.oscars.pce.tce.client.test.TCETestClient $@

