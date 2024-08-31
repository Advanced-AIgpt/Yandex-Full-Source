#!/bin/sh -e

export LOG_DIR=/logs
export HEAP_DUMP_DIR=/var/log/yandex/heapdump
export CORECOUNT=${CORECOUNT:=4}
export XMX=${XMX:=3500m}
export XMS=500m

if ! which java &>/dev/null; then
    export PATH="/home/app/jdk/bin:$PATH"
fi

java -server -showversion \
    -Dquasar.config.path=test/quasar-billing.cfg \
    -Dtvm.config.path=/usr/local/etc/tvm.test.json \
    -Dlogging.config=test/log4j2.xml \
    -agentlib:jdwp=transport=dt_socket,server=y,suspend=n,address=5005 \
    -Dcom.sun.management.jmxremote \
    -Dcom.sun.management.jmxremote.port=1089 \
    -Dcom.sun.management.jmxremote.ssl=false \
    -Dcom.sun.management.jmxremote.authenticate=false \
    -Djava.rmi.server.hostname=localhost \
    -Djava.net.preferIPv6Stack=true \
    -Djava.net.preferIPv4Stack=false \
    -Djava.net.preferIPv6Addresses=true \
    -XX:ActiveProcessorCount=${CORECOUNT} \
    -Dfile.encoding=UTF-8 \
    -Dcom.sun.management.jmxremote.rmi.port=1090 \
    -Xmx${XMX} \
    -Xms${XMS} \
    -XX:+PrintCommandLineFlags \
    -Xlog:gc*:file=${LOG_DIR}/gc.log:time,level,tags:filecount=10,filesize=20M \
    -Xlog:all=warning:file=${LOG_DIR}/jvm.log:time,level,tags:filecount=10,filesize=10m \
    -XX:ErrorFile=${LOG_DIR}/hs_err_pid%p.log \
    -XX:+HeapDumpOnOutOfMemoryError \
    -XX:HeapDumpPath=${HEAP_DUMP_DIR} \
    -verbose:gc \
    -XX:+DisableExplicitGC \
    -XX:+UnlockExperimentalVMOptions \
    -XX:+UseShenandoahGC \
    -XX:MaxGCPauseMillis=20 \
    -XX:+ParallelRefProcEnabled \
    -XX:-UseGCOverheadLimit \
    --add-opens=java.base/java.lang=ALL-UNNAMED \
    -classpath '/home/app/quasar-billing.jar:/home/app/lib/*' \
    ru.yandex.quasar.billing.BillingServer
