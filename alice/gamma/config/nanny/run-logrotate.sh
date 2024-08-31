#! /usr/bin/env bash

mkdir -p /logs
useradd -u 1049 loadbase
[[ -e /var/lib/logrotate/status ]] && [[ ! -e /logs/logrotate.state ]] && cp /var/lib/logrotate/status /logs/logrotate.state
chown loadbase:loadbase /logs
chmod 744 /logs
while true
do
    sleep 3600
    chown loadbase:loadbase /logs/*
    logrotate --state /logs/logrotate.state -v ./logrotate.config
done
