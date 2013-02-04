#!/bin/sh 
#-Djavax.net.debug=ssl:handshake will dump all the ssl messages
DEFAULT_PID_DIR="${OSCARS_HOME-.}/run"
if [ ! -d "$DEFAULT_PID_DIR" ]; then
    mkdir "$DEFAULT_PID_DIR"
fi
case $# in
0) context="DEVELOPMENT";;
1) context=$1;;
esac
echo "Starting TcePCE  with context set to $context"
java -Xmx500m  -Djava.net.preferIPv4Stack=true -jar target/tcePCE-0.0.1-SNAPSHOT.one-jar.jar -c $context &
echo $! > $DEFAULT_PID_DIR/tcePCE.pid
