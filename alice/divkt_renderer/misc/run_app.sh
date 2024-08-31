#!/bin/bash -e

export LOG_DIR=/logs
export HEAP_DUMP_DIR=/dump
export CORECOUNT=${CORECOUNT:=4}
export PARALLELISM=${PARALLELISM:=$CORECOUNT}
export XMX=${XMX:=15G}
export XMS=${XMS:=$XMX}
export METASPACESIZE=${METASPACESIZE:=96m}

if ! which java &>/dev/null; then
    export PATH="./jdk/bin:$PATH"
fi

while true; do
    unified_agent_ready=$(curl localhost:16301/ready 2>/dev/null)
    if [ "$unified_agent_ready" = "OK" ]; then
        echo "Unified Agent is ready"
        break
    else
        echo "Unified Agent is not ready, waiting..."
        sleep 1
    fi
done

# propagate SIGTERM to java
_term() {
  echo "Caught SIGTERM signal!"
  kill -TERM "${child_pid}" 2>/dev/null
}
trap _term SIGTERM

java -server --enable-preview -showversion ${DEBUG_CONFIG} \
    -Dspring.profiles.active="${ENV_TYPE}" \
    -Dlog4j.configurationFile=logging/log4j2-"${ENV_TYPE}".xml \
    -Djava.net.preferIPv6Stack=true \
    -Djava.net.preferIPv4Stack=false \
    -Djava.net.preferIPv6Addresses=true \
    -XX:ActiveProcessorCount=${CORECOUNT} \
    -Djava.util.concurrent.ForkJoinPool.common.parallelism=${PARALLELISM} \
    -Dfile.encoding=UTF-8 \
    -XX:MetaspaceSize=${METASPACESIZE} \
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
    -XX:+UseShenandoahGC \
    -XX:MaxGCPauseMillis=10 \
    -XX:+AlwaysPreTouch \
    -XX:-UseGCOverheadLimit \
    -XX:StartFlightRecording=disk=true,maxsize=100m,filename=${LOG_DIR}/diagnostic_recording.jfr,settings=profile.jfc,dumponexit=true \
    -XX:FlightRecorderOptions=repository=${LOG_DIR}/ \
    -classpath './divkt-renderer/*' \
    ru.yandex.alice.divktrenderer.Application &

# propagate SIGTERM
child_pid="$!"

echo "Java started with pid=${child_pid}"
wait "${child_pid}"
