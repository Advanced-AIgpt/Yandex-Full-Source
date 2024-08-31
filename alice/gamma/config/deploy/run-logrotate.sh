#! /usr/bin/env bash

mkdir -p /logs
useradd -u 1049 loadbase
[[ -e /var/lib/logrotate/status ]] && [[ ! -e /logs/logrotate.state ]] && cp /var/lib/logrotate/status /logs/logrotate.state
chown loadbase:loadbase /logs
chmod 744 /logs
while true
do
    chown loadbase:loadbase /logs/*
    sudo -u loadbase logrotate --state /logs/logrotate.state -v ./logrotate.config
    sleep 3600
done
