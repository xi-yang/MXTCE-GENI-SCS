LIB=lib/
# update classpath
#OSCARS_CLASSPATH=target/test-classes
OSCARS_CLASSPATH=target/test-classes
for f in "$LIB"*.jar
do
 OSCARS_CLASSPATH="$OSCARS_CLASSPATH":$f
done
CLASSPATH=$OSCARS_CLASSPATH
export CLASSPATH=$CLASSPATH
