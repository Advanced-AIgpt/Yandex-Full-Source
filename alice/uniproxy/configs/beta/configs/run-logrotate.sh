#! /usr/bin/env bash

CONFIGS_DIR=$(readlink -f $(dirname $0))

mkdir -p /logs
useradd -u 1049 loadbase
[ -e /var/lib/logrotate/status ] && [ ! -e /logs/logrotate.state ] && cp /var/lib/logrotate/status /logs/logrotate.state
chown loadbase:loadbase /logs
chmod 744 /logs
sleep 120
while true
do
    chown loadbase:loadbase /logs/*
    ./logrotate --state /logs/logrotate.state -v $CONFIGS_DIR/logrotate.conf
    sleep 3600
done

