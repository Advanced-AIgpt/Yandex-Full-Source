#!/bin/bash

set -e

###############################################################################
#
#   Configure vars
#
UNIPROXY_WRAPPER="/usr/bin/yandex-voice-uniproxy-wrapper"
UNISTAT_WRAPPER="/usr/bin/yandex-voice-unistat-wrapper"
UNIPROXY_SUBWAY_WRAPPER="/usr/bin/yandex-voice-subway-wrapper"
UNIPROXY_INSTALL_DIR="/usr/lib/yandex/voice/uniproxy"
EXTRA_DEBUG_PACKAGES="vim mc net-tools"

cp -r /etc/supervisor /etc/supervisor_qloud
sed -i -e 's@/etc/supervisor@/etc/supervisor_qloud@g' /etc/supervisor_qloud/supervisord.conf

###############################################################################
#
#   Generate UNI proxy wrapper and supervisor config
#
cat <<EOF >>${UNIPROXY_WRAPPER}
#!/bin/bash
/usr/bin/uniproxy --port \${1:-80} &
UNIPROXY_PID="\$!"

function stop_uniproxy() {
    PIDS=\$(pstree -p \${UNIPROXY_PID} | grep -oe "([0-9]\\+)" | grep -oe "[0-9]\\+")
    kill -TERM \${PIDS}
}

trap "stop_uniproxy" EXIT TERM INT

wait \${UNIPROXY_PID}
EOF
chmod +x ${UNIPROXY_WRAPPER}

cat <<EOF >>/etc/supervisor/conf.d/uniproxy.conf
[program:uniproxy]
command = ${UNIPROXY_WRAPPER} 80
autostart = true
autorestart = true
stderr_logfile=/logs/uniproxy.err
stderr_logfile_maxbytes=50MB
stderr_logfile_backups=2
stdout_logfile=/logs/uniproxy.out
stdout_logfile_maxbytes=1GB
stdout_logfile_backups=2
EOF

###############################################################################
#
#   Generate Subway config
#
cat <<EOF >>${UNIPROXY_SUBWAY_WRAPPER}
#!/bin/bash
/usr/bin/uniproxy-subway &
SUBWAY_PID="\$!"

function stop_subway() {
    kill -TERM \${SUBWAY_PID}
}

trap "stop_subway" EXIT TERM INT

wait \${SUBWAY_PID}
EOF
chmod +x ${UNIPROXY_SUBWAY_WRAPPER}

cat <<EOF >>/etc/supervisor/conf.d/subway.conf
[program:subway]
command = ${UNIPROXY_SUBWAY_WRAPPER}
autostart = true
autorestart = true
stderr_logfile=/logs/subway.err
stderr_logfile_maxbytes=50MB
stderr_logfile_backups=2
stdout_logfile=/logs/subway.out
stdout_logfile_maxbytes=500MB
stdout_logfile_backups=2
EOF


################################################################################

###############################################################################
#
#   Configure vars
#
QLOUD_UNIPROXY_WRAPPER="/usr/bin/yandex-voice-uniproxy-wrapper-qloud"
QLOUD_UNISTAT_WRAPPER="/usr/bin/yandex-voice-unistat-wrapper-qloud"
QLOUD_UNIPROXY_SUBWAY_WRAPPER="/usr/bin/yandex-voice-subway-wrapper-qloud"


###############################################################################
#
#   Generate UNI proxy wrapper and supervisor config
#
cat <<EOF >>${QLOUD_UNIPROXY_WRAPPER}
#!/bin/bash
/usr/bin/uniproxy --port \${1:-80} &
UNIPROXY_PID="\$!"

function stop_uniproxy() {
    PIDS=\$(pstree -p \${UNIPROXY_PID} | grep -oe "([0-9]\\+)" | grep -oe "[0-9]\\+")
    kill -TERM \${PIDS}
}

trap "stop_uniproxy" EXIT TERM INT

wait \${UNIPROXY_PID}
EOF
chmod +x ${QLOUD_UNIPROXY_WRAPPER}

cat <<EOF >>/etc/supervisor_qloud/conf.d/uniproxy.conf
[program:uniproxy]
command = ${QLOUD_UNIPROXY_WRAPPER} 80
autostart = true
autorestart = true
stderr_logfile=/dev/stderr
stderr_logfile_maxbytes=0
stdout_logfile=/dev/stdout
stdout_logfile_maxbytes=0
EOF

###############################################################################
#
#   Generate Subway config
#
cat <<EOF >>/etc/supervisor_qloud/conf.d/subway.conf
[program:subway]
command = /usr/bin/uniproxy-subway
autostart = true
autorestart = true
stderr_logfile=/dev/stderr
stderr_logfile_maxbytes=0
stdout_logfile=/dev/stdout
stdout_logfile_maxbytes=0
EOF

##############################################################################
#
#   Generate unistat proxy wrapper and supervisor config
#
cat <<EOF >${QLOUD_UNISTAT_WRAPPER}
#!/bin/bash
/usr/bin/uniproxy-unistat --port \${1:-8800} --ports \${2:-80,7777} &
UNISTAT_PID="\$!"

function stop_unistat() {
    PIDS=\$(pstree -p \${UNISTAT_PID} | grep -oe "([0-9]\\+)" | grep -oe "[0-9]\\+")
    kill -TERM \${PIDS}
}

trap "stop_unistat" EXIT TERM INT

wait \${UNISTAT_PID}
EOF
chmod +x ${QLOUD_UNISTAT_WRAPPER}

cat <<EOF >/etc/supervisor_qloud/conf.d/unistat.conf
[program:unistat]
command = ${QLOUD_UNISTAT_WRAPPER} 8800 7777,80
autostart = true
autorestart = true
stderr_logfile=/dev/null
stderr_logfile_maxbytes=0
stdout_logfile=/dev/null
stdout_logfile_maxbytes=0
EOF

