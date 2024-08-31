#!/usr/bin/env bash

mkdir -p /logs
useradd -u 1049 loadbase
[[ -e /var/lib/logrotate/status ]] && [[ ! -e /logs/logrotate.state ]] && cp /var/lib/logrotate/status /logs/logrotate.state
chown loadbase:loadbase /logs
chmod 744 /logs
while true; do
    sleep 300
    sudo chown loadbase:loadbase /logs/*
    sudo --user=loadbase logrotate --state /logs/logrotate.state -v ./nanny/logrotate.conf
done
