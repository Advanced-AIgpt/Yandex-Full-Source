#! /usr/bin/env bash

mkdir -p /logs
mkdir -p ./pids

[ -e /var/lib/logrotate/status ] && [ ! -e /logs/logrotate.state ] && cp /var/lib/logrotate/status /logs/logrotate.state

chmod 744 /logs
sleep 120

while true
do
    chown -R loadbase:loadbase /logs
    ./logrotate --state /logs/logrotate.state -v $CONFIGS_DIR/logrotate.conf
    sleep 600
done
