#!/bin/bash

PACKAGE_DIR=`dirname "$0"`
UNIPROXY_WRAPPER="$SWD/uniproxy-wrapper"
UNIPROXY_SUBWAY_WRAPPER="$SWD/subway-wrapper"
PYTHON_UNIPROXY_PORT=${PYTHON_UNIPROXY_PORT:-80}

###############################################################################
#
#   Generate UNI proxy wrapper and supervisor config
#

if [[ -f $SWD/uniproxy_run.sh ]]; then
    RUN="$SWD/uniproxy_run.sh"
    echo "Use custom script to run UniProxy"
else
    RUN="$PACKAGE_DIR/uniproxy --port \${1:-80} &"
fi
echo "UniProxy will be started as: $RUN"

cat <<EOF >>${UNIPROXY_WRAPPER}
#!/bin/bash

$RUN

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
command = ${UNIPROXY_WRAPPER} ${PYTHON_UNIPROXY_PORT}
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
$PACKAGE_DIR//uniproxy-subway &
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
