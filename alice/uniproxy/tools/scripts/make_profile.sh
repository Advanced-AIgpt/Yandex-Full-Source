#!/bin/bash
HOSTNAME=$1
PREFIX=$2
CLOCK_TYPE=${3:-"cpu"}
PERFTESTER="`dirname $0`/../perf_tester/perf_tester"
AMMO_YT_TABLE='//home/alice/wsnk/ya.station[:#2000]'


START_URL="http://$HOSTNAME/start_profiling?clock=$CLOCK_TYPE"
STOP_URL="http://$HOSTNAME/stop_profiling"


if [ ! -x $PERFTESTER ]; then
    echo "perf_tester is absent" >&2
    exit 1
fi


# warmup
WARMUP_AMMO_YT_TABLE='//home/alice/wsnk/ya.station[:#20]'
if ! YT_PROXY=hahn $PERFTESTER --yt-table "$WARMUP_AMMO_YT_TABLE" --url "ws://$HOSTNAME/uni.ws" >/dev/null ; then
    echo "warmup failed" >&2
    exit 1
fi

sleep 5

if ! curl --fail $START_URL ; then
    echo "Couldn't enable profiling on \"$HOSTNAME\"" >&2
    exit 1
fi

if ! YT_PROXY=hahn $PERFTESTER --yt-table "$AMMO_YT_TABLE" --url "ws://$HOSTNAME/uni.ws" > ./${PREFIX}perftest.json ; then
    echo "perf-tester failed" >&2
    exit 1
fi

while ! curl --fail $STOP_URL; do
    :
done
ssh $HOSTNAME "cat /logs/uniproxy_profile.callgrind" > ./${PREFIX}uniproxy_profile.callgrind
