LIB=lib/
# update classpath
OSCARS_CLASSPATH=target/test-classes:target/classes:target/tceApiClient-0.0.1-SNAPSHOT.jar
for f in "$LIB"*.jar
do
 OSCARS_CLASSPATH="$OSCARS_CLASSPATH":$f
done
CLASSPATH=$OSCARS_CLASSPATH
export CLASSPATH=$CLASSPATH
