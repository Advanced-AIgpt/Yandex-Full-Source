#!/bin/bash

function start() {
    URL="http://$HOSTNAME/start_profiling?clock=${CLOCK:-cpu}"
    if ! curl --fail $URL ; then
        echo "Couldn't enable profiling on \"$HOSTNAME\"" >&2
        exit 1
    fi
}

function stop() {
    while ! curl --fail "http://$HOSTNAME/stop_profiling" 2>/dev/null; do
        :
    done
    ssh $HOSTNAME "cat /logs/uniproxy_profile.callgrind" > ./${PREFIX}uniproxy_profile.callgrind
}

function usage() {
    echo "Start or stop uniproxy profiler on a given host
Usage: $0 <start|stop> --host <HOSTNAME> [--clock <cpu|wall>] [--prefix <SAVED_PROFILE_PREFIX>]
" >&2
}


CMD=$1
shift

while [[ $# != 0 ]]; do
    case $1 in
    "--host")   HOSTNAME=$2;;
    "--clock")  CLOCK=$2;;
    "--prefix") PREFIX=$2;;
    esac
    shift 2
done

case $CMD in
    "start") start;;
    "stop") stop;;
    "") usage;;
esac

