#!/bin/bash -e
set -e

LOG_DIR=/logs
HEAP_DUMP_DIR=/dump
export CORECOUNT=${CORECOUNT:=4}
export XMX=${XMX:=14G}
export XMS=${XMS:=$XMX}
ENVIRONMENT="dev"
DEBUG_PORT=""
TVM_MODE=${TVM_MODE:="tvmtool"}
HTTP_PORT=${HTTP_PORT:=80}

#Read properties from script keys
for opt in "$@"; do
  case ${opt} in
    --environment=*) ENVIRONMENT="${opt#*=}"
    shift ;;
    --cpu-count=*) CORECOUNT="${opt#*=}"
    shift ;;
    --logdir=*) LOG_DIR="${opt#*=}"
    shift ;;
    --debug-port=*) DEBUG_PORT="${opt#*=}"
    shift ;;
    --jmx-port=*) JMX_PORT="${opt#*=}"
    shift ;;
    --tvm-mode=*) TVM_MODE="${opt#*=}"
    shift ;;
    --)
    shift ; break ;;
    *) echo Warning: Skipping unknown parameter $1
    shift ;;
  esac
done


DEBUG_OPTS=""
if [[ -n "${DEBUG_PORT}" ]] ; then
  DEBUG_OPTS=" -agentlib:jdwp=transport=dt_socket,server=y,suspend=n,address=${DEBUG_PORT}"
fi

JMX_OPTS=""
if [[ -n "${JMX_PORT}" ]] ; then
  JMX_OPTS=" -Dcom.sun.management.jmxremote
                -Dcom.sun.management.jmxremote.port=${JMX_PORT}
                -Dcom.sun.management.jmxremote.ssl=false
                -Dcom.sun.management.jmxremote.authenticate=false
                -Dcom.sun.management.jmxremote.rmi.port=${JMX_PORT}
                -Djava.rmi.server.hostname=localhost"
fi

if ! which java &>/dev/null; then
    export PATH="/home/app/jdk/bin:$PATH"
fi

echo "Starting dialogovo. Environment: ${ENVIRONMENT}"

java -server -showversion ${DEBUG_CONFIG} \
    -Djava.library.path=/home/app/lib/ \
    -Dspring.profiles.active=${ENVIRONMENT} \
    -Dserver.jetty.threads.min=${MIN_JETTY_THREADS:-50} \
    -Dlog4j.configurationFile=${ENVIRONMENT}/log4j2.xml \
    -Dtvm.mode=${TVM_MODE} \
    ${DEBUG_OPTS} \
    ${JMX_OPTS} \
    ${ADDITIONAL_OPTS} \
    -Djava.net.preferIPv6Stack=true \
    -Djava.net.preferIPv4Stack=false \
    -Djava.net.preferIPv6Addresses=true \
    -XX:ActiveProcessorCount=${CORECOUNT} \
    -XX:+PrintCompilation \
    -Dfile.encoding=UTF-8 \
    -Dserver.port=${HTTP_PORT} \
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
    -XX:+AlwaysActAsServerClassMachine \
    -XX:+UnlockExperimentalVMOptions \
    -XX:+UseShenandoahGC \
    -XX:MaxGCPauseMillis=10 \
    -XX:+ParallelRefProcEnabled \
    -XX:-UseGCOverheadLimit \
    -XX:StartFlightRecording=disk=true,maxsize=100m,filename=${LOG_DIR}/diagnostic_recording.jfr,settings=profile.jfc,dumponexit=true \
    -XX:FlightRecorderOptions=repository=${LOG_DIR}/ \
    --add-opens=java.base/java.lang=ALL-UNNAMED \
    -classpath '/home/app/dialogovo.jar:/home/app/lib/*' \
    ru.yandex.alice.kronstadt.runner.KronstadtApplication
