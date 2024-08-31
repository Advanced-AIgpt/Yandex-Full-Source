#!/bin/bash
# Script to run tcpdump via SSH on multiple remote hosts and store dumps locally

HOSTS=(  # add your remote hosts here
    man1-0633-man-uniproxy-20233.gencfg-c.yandex.net
    man1-0994-man-uniproxy-20233.gencfg-c.yandex.net
)
DEST_DIR="/opt/raid6/dumps"  # directory to store pcaps
EXPRESSION="tcp port 80"  # tcpdump's expression
PIDS_FILE="$DEST_DIR/.dump.pids"  # file to save PIDs of launched processes 


#--------------------------------------------------------------------------------------------------
function start()
{
    echo "Start dumpings..." >&2
    for HOST in "${HOSTS[@]}"; do
        ssh -n -oStrictHostKeyChecking=no $HOST "tcpdump -i any -w - \"$EXPRESSION\" | gzip" > "$DEST_DIR/$HOST.pcap" &
        if [ $? != 0 ]; then
            echo "Failed to run tcpdump on $HOST" >&2
        else
            echo "Run tcpdump on $HOST" >&2
            echo $! >> $PIDS_FILE
        fi
    done
}


#--------------------------------------------------------------------------------------------------
function stop()
{
    echo "Stop dumpings..." >&2
    if [ -f $PIDS_FILE ]; then
        kill `cat $PIDS_FILE`
        rm -f $PIDS_FILE
    fi
}


#--------------------------------------------------------------------------------------------------
if [ "$1" == "stop" ]; then
    stop
else
    start
fi
