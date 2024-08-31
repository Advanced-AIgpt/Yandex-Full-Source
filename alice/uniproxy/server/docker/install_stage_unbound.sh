#!/bin/bash
set -e
########################
#    Unbound config    #
########################
cat << EOF > /etc/unbound/unbound.conf
server:
    module-config: "iterator"
    interface: 127.0.0.1
    interface: ::1
    access-control: 127.0.0.0/8 allow_snoop
    access-control: ::1/128 allow_snoop
    prefetch: yes
    so-reuseport: yes
    local-zone: "localhost.yandex.ru." static
    local-data: "localhost.yandex.ru. 10800 IN NS localhost."
    local-data: "localhost.yandex.ru. 10800 IN A 127.0.0.1"
    local-data: "localhost.yandex.ru. 10800 IN AAAA ::1"
    local-zone: "localhost.yandex.net." static
    local-data: "localhost.yandex.net. 10800 IN NS localhost."
    local-data: "localhost.yandex.net. 10800 IN A 127.0.0.1"
    local-data: "localhost.yandex.net. 10800 IN AAAA ::1"

remote-control:
    control-enable: yes

forward-zone:
    name: "."
    forward-addr: 2a02:6b8:0:3400::5005
EOF

cat << EOF > /etc/resolv.conf
nameserver ::1
nameserver 2a02:6b8:0:3400::5005
search yandex.net yandex.ru
options timeout:1 attempts:1
EOF

cat <<EOF > /etc/supervisor_qloud/conf.d/unbound.conf
[program:unbound]
command = /usr/sbin/unbound -d
autostart = true
autorestart = true
stderr_logfile=/var/run/qloud/unbound.err
stderr_logfile_maxbytes=50MB
stderr_logfile_backups=2
stdout_logfile=/var/run/qloud/unbound.out
stdout_logfile_maxbytes=500MB
stdout_logfile_backups=2
EOF

cat <<EOF > /etc/supervisor/conf.d/unbound.conf
[program:unbound]
command = /usr/sbin/unbound -d
autostart = true
autorestart = true
stderr_logfile=/logs/unbound.err
stderr_logfile_maxbytes=50MB
stderr_logfile_backups=2
stdout_logfile=/logs/unbound.out
stdout_logfile_maxbytes=500MB
stdout_logfile_backups=2
EOF
