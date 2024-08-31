#!/bin/sh -e

export LOG_DIR=/logs
export HEAP_DUMP_DIR=/var/log/yandex/my_alice_pumpkin/heapdump
export CORECOUNT=${CORECOUNT:=1}
export XMX=${XMX:=2G}
export XMS=${XMS:=2G}

java -server -showversion \
    -agentlib:jdwp=transport=dt_socket,server=y,suspend=n,address=5005 \
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
    -XX:MaxGCPauseMillis=20 \
    -classpath '/home/app/my_alice_pumpkin.jar:/home/app/lib/*' \
    -Dspring.profiles.active="${ENV_TYPE}" \
    -Dspring.config.location=./config/ \
    -Dlog4j.configurationFile=./config/logging/log4j2-dev.xml \
    ru.yandex.alice.paskills.my_alice.pumpkin.Application
