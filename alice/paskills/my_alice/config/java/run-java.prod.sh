#!/bin/sh -e

export LOG_DIR=/logs
export HEAP_DUMP_DIR=/var/log/yandex/my_alice/heapdump
export CORECOUNT=${CORECOUNT:=4}
export XMX=${XMX:=15G}
export XMS=${XMS:=$XMX}
export GEOBASE_TZ_PATH=/home/app/geobase/data/tzdata/zones_bin

java -server -showversion \
    -Djava.library.path=/home/app/lib/ \
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
    -verbose:gc \
    -XX:+DisableExplicitGC \
    -XX:+UnlockExperimentalVMOptions \
    -XX:+UseShenandoahGC \
    -XX:MaxGCPauseMillis=10 \
    -XX:+ParallelRefProcEnabled \
    -XX:-UseGCOverheadLimit \
    -classpath '/home/app/my_alice.jar:/home/app/lib/*' \
    -Dspring.profiles.active="${ENV_TYPE}" \
    -Dspring.config.location=./config/ \
    ru.yandex.alice.paskills.my_alice.Application
