#! /usr/bin/env bash

CONFIGS_DIR=$(readlink -f $(dirname $0))

mkdir -p /logs

[ -e /var/lib/logrotate/status ] && [ ! -e /logs/logrotate.state ] && cp /var/lib/logrotate/status /logs/logrotate.state

chmod 744 /logs
while true
do
    chown -R loadbase:loadbase /logs
    ./logrotate --state /logs/logrotate.state -v $CONFIGS_DIR/logrotate.conf
    sleep 1800
done

