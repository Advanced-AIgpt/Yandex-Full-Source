#!/bin/bash

set -e

###############################################################################
#
#   Configure vars
#
UNIDELIVERY_WRAPPER="/usr/bin/yandex-voice-unidelivery-wrapper"
UNIPROXY_INSTALL_DIR="/usr/lib/yandex/voice/uniproxy"
EXTRA_DEBUG_PACKAGES="vim mc net-tools"

cp -r /etc/supervisor /etc/supervisor_qloud
sed -i -e 's@/etc/supervisor@/etc/supervisor_qloud@g' /etc/supervisor_qloud/supervisord.conf

###############################################################################
#
#   Generate UniDelivery config
#
cat <<EOF >>${UNIDELIVERY_WRAPPER}
#!/bin/bash
/usr/bin/uniproxy-delivery -p ${QLOUD_HTTP_PORT:-80} -n 8 &
UNIDELIVERY_PID="\$!"

function stop_unidelivery() {
    PIDS=\$(pstree -p \${UNIDELIVERY_PID} | grep -oe "([0-9]\\+)" | grep -oe "[0-9]\\+")
    kill -TERM \${PIDS}
}

trap "stop_unidelivery" EXIT TERM INT

wait \${UNIDELIVERY_PID}
EOF
chmod +x ${UNIDELIVERY_WRAPPER}

cat <<EOF >>/etc/supervisor_qloud/conf.d/unidelivery.conf
[program:yandex-uni-delivery]
command = ${UNIDELIVERY_WRAPPER}
autostart = true
autorestart = true
stderr_logfile=/dev/stderr
stderr_logfile_maxbytes=0
stdout_logfile=/dev/stdout
stdout_logfile_maxbytes=0
EOF


################################################################################

#cd /usr/lib/yandex/voice/uniproxy && make check
