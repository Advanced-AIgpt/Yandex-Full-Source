#!/usr/bin/env bash

[[ -e /var/lib/logrotate/status ]] && [[ ! -e /logs/logrotate.state ]] && cp /var/lib/logrotate/status /logs/logrotate.state
useradd -u 1049 loadbase
usermod -a -G root loadbase
chmod 744 /logs
chown loadbase:loadbase /logs
while true; do
    sleep 150
    chown loadbase:loadbase /logs/*
    su - loadbase -c "logrotate --state /logs/logrotate.state -v /usr/local/etc/logrotate.conf"
done
