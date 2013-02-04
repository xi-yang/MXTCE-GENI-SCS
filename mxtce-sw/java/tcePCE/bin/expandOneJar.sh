#!/bin/sh
rm -rf target/tmp
mkdir target/tmp
cp config/log4j.*.properties target/classes
(cd target/tmp; jar -xf ../tcePCE-0.0.1-SNAPSHOT.one-jar.jar )
