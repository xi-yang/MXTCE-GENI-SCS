#!/bin/sh
#usage test.sh 
. bin/setclasspath.sh
OSCARS_HOME=`pwd`
java net.es.oscars.pce.tce.client.test.TCETestClient $@

