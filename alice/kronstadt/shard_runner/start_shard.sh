#!/bin/bash

ENVIRONMENT="dev"
PORT=8000
GRPC_PORT=10000
OUTPUT="./kronstadt.log"
TVM_URL=http://localhost:8001
TVM_MODE=tvmapi
DIALOGOVO_PROD_TVM=2015309
TVM_CLIENT_ID=$DIALOGOVO_PROD_TVM
YAV_SECRET=ver-01dq7mqmeh87drw0bsdm6xt3sn

function Help() {

    GREEN=`tput setaf 2`
    YELLOW=`tput setaf 3`
    NC=`tput sgr0`

    # Display help
    echo "Script to start kronstadt server"
    echo ""
    echo "Main arguments"
    echo "${GREEN}--scenario-path=<path>${NC}    full path to scenario module in arcadia from filesystem root"
    echo "${GREEN}--environment=<profile>>${NC}  set active spring profile (default $ENVIRONMENT)"
    echo "${GREEN}--port=<port>${NC}             set http port (default $PORT)"
    echo "${GREEN}--grpc-port=<grpc_port>${NC}   set http port (default $GRPC_PORT)"
    echo "${GREEN}--tvm-mode=tvmapi${NC}         set tvm mode: tvmapi/tvmtool/disabled"
    echo ""
    echo "For tvm-mode=tvmapi"
    echo "tvmapi credentials may be provided via TVM_CLIENT_ID and TVM_SECRET for security reasons."
    echo "${GREEN}--tvm-client-id=<id>${NC}      set Self TVM client ID (defaults TVM_CLIENT_ID)"
    echo "${GREEN}--tvm-secret=<secret>${NC}     set TVM secret, value from yav (defaults $YAV_SECRET)"
    echo ""
    echo "For tvm-mode=tvmtool"
    echo "${GREEN}--tvm-url=${NC}                set tvmtool url (default $TVM_URL)"
    echo "${GREEN}--tvm-token=<token>${NC}       set TVM token (default to content of \"~/tvmtool/token.txt\"  file)"
    echo ""
    echo "Additional options"
    echo "${YELLOW}--output=<file path>${NC}     set log file (default $OUTPUT)"
    echo "${YELLOW}--disable-tvm${NC}            use fake TVM client instead of real one. Usefull for local execution  (default real TVM used)"
    echo "${YELLOW}--debug${NC}                  enable debug on 5005 port (default disables)"
    echo "${YELLOW}--debug-port=<port>${NC}      enable debug on specified port (default disables)"
    echo "${YELLOW}--jmx${NC}                    enable jmx on 1089 port (default disables)"
    echo "${YELLOW}--jmx-port=<port>${NC}        enable jmx on specified port (default disables)"
}

for opt in "$@"; do
    case ${opt} in
      --help)
          Help
          exit ;;
      -h)
          Help
          exit ;;
    esac
done


TVM_TOKEN=`cat ~/tvmtool/token.txt 2> /dev/null`
DEBUG_OPTS=""
JMX_OPTS=""
SCENARIOS=""
NL=$'\n'

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

function get-arcadia-root {
    local R="$DIR"
    while [[ ! -e "${R}/.arcadia.root" ]]; do
        if [[ "${R}" == "/" ]]; then
            echo "$0: must be run from inside Arcadia checkout" >&2
            exit  1
        fi
        R="$(dirname "${R}")"
    done
    echo "$R"
}

ARCADIA_ROOT="${ARCADIA_ROOT:-$(get-arcadia-root)}"
YA_BIN=${YA_BIN:-$ARCADIA_ROOT/ya}
DISABLE_TVM="N"
echo "Starting Kronstadt shard with arguments: $@"

#Read properties from script keys
for opt in "$@"; do
  case ${opt} in
    --environment=*) ENVIRONMENT="${opt#*=}"
    shift ;;
    --debug) DEBUG_PORT="5005"
    shift ;;
    --debug-port=*) DEBUG_PORT="${opt#*=}"
    shift ;;
    --jmx) JMX_PORT="1089"
    shift ;;
    --jmx-port=*) JMX_PORT="${opt#*=}"
    shift ;;
    --port=*) PORT="${opt#*=}"
    shift ;;
    --grpc_port=*) GRPC_PORT="${opt#*=}"
    shift ;;
    --output=*) OUTPUT="${opt#*=}"
    shift ;;
    --tvm-url=*) TVM_URL="${opt#*=}"
    shift ;;
    --tvm-token=*) TVM_TOKEN="${opt#*=}"
    shift ;;
    --tvm-mode=*) TVM_MODE="${opt#*=}"
    shift ;;
    --tvm-client-id=*) TVM_CLIENT_ID="${opt#*=}"
    shift ;;
    --tvm-secret=*) TVM_SECRET="${opt#*=}"
    shift ;;
    --scenario_path=*) SC_PATH=${opt#*=} && SCENARIOS="${SCENARIOS}  ${SC_PATH#$ARCADIA_ROOT/}${NL}"
    shift ;;
    --scenario-path=*) SC_PATH=${opt#*=} && SCENARIOS="${SCENARIOS}  ${SC_PATH#$ARCADIA_ROOT/}${NL}"
    shift ;;
    --disable-tvm) DISABLE_TVM=Y
    shift ;;
    --help)
    shift ;;
    --)
    shift ; break ;;
    *) echo Warning: Skipping unknown parameter $opt
    shift ;;
  esac
done

if [[ -z "${SCENARIOS}" ]]; then
    echo "No --scenario-path specified."
    exit 1
fi

DISABLE_TVM_ARG=""
if [[ $DISABLE_TVM == 'Y' ]]; then
    DISABLE_TVM_ARG="-Dtvm.mode=disabled"
fi

if [[ -n "${DEBUG_PORT}" ]] ; then
  DEBUG_OPTS=" -agentlib:jdwp=transport=dt_socket,server=y,suspend=n,address=${DEBUG_PORT}"
fi

if [[ -n "${JMX_PORT}" ]] ; then
  JMX_OPTS=" -Dcom.sun.management.jmxremote
                -Dcom.sun.management.jmxremote.port=${JMX_PORT}
                -Dcom.sun.management.jmxremote.ssl=false
                -Dcom.sun.management.jmxremote.authenticate=false
                -Dcom.sun.management.jmxremote.rmi.port=${JMX_PORT}
                -Djava.rmi.server.hostname=localhost"
fi
echo "PEERDIR(${NL}${SCENARIOS})" > "$DIR/scenarios.inc"


echo "PORT:" $PORT
echo "GRPC_PORT:" $GRPC_PORT
echo "ARCADIA_ROOT:" $ARCADIA_ROOT
echo "JAVA_HOME:" $JAVA_HOME

if [[ "$TVM_MODE" == "tvmapi" ]] ; then
    if [ -z "$TVM_CLIENT_ID" ] ; then
        echo "[ERROR] TVM_CLIENT_ID variable must be set in TvmApi mode"
        exit 1
    fi
    if [ -z "$TVM_SECRET" ] ; then
        if [[ "$TVM_CLIENT_ID" == "$DIALOGOVO_PROD_TVM" ]] ; then
            echo "Fetching TVM secret for TVM_CLIENT_ID=$TVM_CLIENT_ID"
            TVM_SECRET=$(ya vault get version ${YAV_SECRET} --only-value client_secret)
        else
            echo "[ERROR] TVM_SECRET variable must be set in TvmApi mode"
            exit 1
        fi
    fi
elif [[ "$TVM_MODE" == "tvmtool" ]]; then
    if [ -z "$TVM_URL" ] ; then
        echo "[ERROR] TVM_URL variable must be set in TvmApi mode"
        exit 1
    fi
    if [ -z "$TVM_TOKEN" ] ; then
        echo "[ERROR] TVM_TOKEN variable must be set in TvmApi mode"
        exit 1
    fi
fi

# build shard and all its scenarios
cd $DIR
echo "Recompiling shard"
rm -r $ARCADIA_ROOT/alice/kronstadt/shard_runner/kronstadt-shard-runner 2> /dev/null
time $YA_BIN make --show-timings --stat -r -DKRONSTADT_SCENARIO_INC="${DIR#$ARCADIA_ROOT/}/scenarios.inc" "$ARCADIA_ROOT/alice/kronstadt/shard_runner"

echo "Starting shard. Logging to $OUTPUT"
$ARCADIA_ROOT/alice/kronstadt/shard_runner/jdk/bin/java -server -showversion \
       -XX:+HeapDumpOnOutOfMemoryError \
       -Dfile.encoding=UTF-8 \
       -XX:+PrintCommandLineFlags \
       -XX:+AlwaysActAsServerClassMachine \
       -XX:+UseShenandoahGC \
       --add-opens=java.base/java.lang=ALL-UNNAMED \
       $DEBUG_OPTS \
       -Dserver.port=$PORT \
       -Dapphost.port={$GRPC_PORT} \
       -Dspring.profiles.active=${ENVIRONMENT} \
       -Dtvm-mode=${TVM_MODE} \
       -Dtvm.url=${TVM_URL} \
       -Dtvm.token=${TVM_TOKEN} \
       -Dtvm.selfClientId=${TVM_CLIENT_ID} \
       -Dtvm.secret=${TVM_SECRET} \
       -Dapphost.port=${GRPC_PORT} \
       -Dlogging.config=classpath:log4j2-${ENVIRONMENT}.xml \
       $DISABLE_TVM_ARG \
       $ADDITIONAL_OPTIONS \
       -Djava.library.path="$ARCADIA_ROOT/alice/kronstadt/shard_runner/kronstadt-shard-runner" \
       -Djava.jni.path="$ARCADIA_ROOT/alice/kronstadt/shard_runner/kronstadt-shard-runner" \
       -cp "$ARCADIA_ROOT/alice/kronstadt/shard_runner/kronstadt-shard-runner/*" \
       ru.yandex.alice.kronstadt.runner.KronstadtApplication >> $OUTPUT
