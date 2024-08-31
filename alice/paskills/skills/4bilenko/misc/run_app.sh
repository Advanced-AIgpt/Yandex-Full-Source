#!/bin/sh -e

cd /home/app

export LOG_DIR=/logs
export HEAP_DUMP_DIR=/dump
export CORECOUNT=${CORECOUNT:=1}
export XMX=${XMX:=1G}
export XMS=${XMS:=$XMX}

java -server --enable-preview -showversion ${DEBUG_CONFIG} \
    -Djava.net.preferIPv6Stack=true \
    -Djava.net.preferIPv4Stack=false \
    -Djava.net.preferIPv6Addresses=true \
    -XX:ActiveProcessorCount=${CORECOUNT} \
    -Dfile.encoding=UTF-8 \
    -Xmx${XMX} \
    -Xms${XMS} \
    -XX:+PrintCommandLineFlags \
    -Xlog:gc+stats:file=${LOG_DIR}/gc_stats.log:time,level,tags:filecount=10,filesize=20M \
    -Xlog:gc*:file=${LOG_DIR}/gc.log:time,level,tags:filecount=10,filesize=20M \
    -Xlog:all=warning:file=${LOG_DIR}/jvm.log:time,level,tags:filecount=10,filesize=10m \
    -XX:ErrorFile=${LOG_DIR}/hs_err_pid%p.log \
    -XX:+HeapDumpOnOutOfMemoryError \
    -XX:HeapDumpPath=${HEAP_DUMP_DIR} \
    -XX:+DisableExplicitGC \
    -XX:+UnlockExperimentalVMOptions \
    -XX:+UseShenandoahGC \
    -XX:MaxGCPauseMillis=10 \
    -XX:+AlwaysPreTouch \
    -XX:-UseGCOverheadLimit \
    -XX:-UseBiasedLocking \
    -classpath './bilenko_skill.jar:./app_libs/*' \
    ru.yandex.alice.paskills.skills.bilenko.Application
