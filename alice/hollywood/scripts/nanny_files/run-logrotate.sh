#!/usr/bin/env bash

useradd -u 1049 loadbase
[[ -e /var/lib/logrotate/status ]] && [[ ! -e ./logs/logrotate.state ]] && cp /var/lib/logrotate/status ./logs/logrotate.state
chown loadbase:loadbase ./logs
chmod 744 ./logs
chmod 644 ./logrotate.conf
while true; do
    sleep 30
    sudo chown loadbase:loadbase ./logs/*
    sudo -u loadbase logrotate --state ./logs/logrotate.state -v ./logrotate.conf
done
