#!/bin/sh -e

export LOG_DIR=/logs
export HEAP_DUMP_DIR=/var/log/yandex/my_alice_pumpkin/heapdump
export CORECOUNT=${CORECOUNT:=1}
export XMX=${XMX:=2G}
export XMS=${XMS:=$XMX}

java -server -showversion \
    -Djava.library.path=/home/app/lib/ \
    -agentlib:jdwp=transport=dt_socket,server=y,suspend=n,address=5005 \
    -Dcom.sun.management.jmxremote \
    -Dcom.sun.management.jmxremote.port=1089 \
    -Dcom.sun.management.jmxremote.ssl=false \
    -Dcom.sun.management.jmxremote.authenticate=false \
    -Dcom.sun.management.jmxremote.rmi.port=1090 \
    -Djava.rmi.server.hostname=localhost \
    -Djava.net.preferIPv6Stack=true \
    -Djava.net.preferIPv4Stack=false \
    -Djava.net.preferIPv6Addresses=true \
    -XX:ActiveProcessorCount=${CORECOUNT} \
    -Dfile.encoding=UTF-8 \
    -Xmx${XMX} \
    -Xms${XMS} \
    -XX:+PrintCommandLineFlags \
    -Xlog:gc*:file=${LOG_DIR}/gc.log:time,level,tags:filecount=10,filesize=20M \
    -Xlog:all=warning:file=${LOG_DIR}/jvm.log:time,level,tags:filecount=10,filesize=10m \
    -XX:ErrorFile=${LOG_DIR}/hs_err_pid%p.log \
    -XX:+HeapDumpOnOutOfMemoryError \
    -XX:HeapDumpPath=${HEAP_DUMP_DIR} \
    -XX:+DisableExplicitGC \
    -XX:+UnlockExperimentalVMOptions \
    -XX:+UseShenandoahGC \
    -XX:MaxGCPauseMillis=10 \
    -XX:+ParallelRefProcEnabled \
    -XX:-UseGCOverheadLimit \
    -classpath '/home/app/my_alice_pumpkin.jar:/home/app/lib/*' \
    -Dspring.profiles.active="${ENV_TYPE}" \
    -Dspring.config.location=./config/ \
    -Dlog4j.configurationFile=./config/logging/log4j2-hamster.xml \
    ru.yandex.alice.paskills.my_alice.pumpkin.Application
