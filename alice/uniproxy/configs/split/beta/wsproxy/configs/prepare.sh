#!/usr/bin/env bash

mkdir -p /logs/supervisor

# prepare TVM
xxd -l 16 -p /dev/urandom > ./token.txt

# directory need for apphost ITS (backend patching)
mkdir -p .update_from_fs_tmp_dir
mkdir -p backends-after-its

# make link for correct wrappers
ln --symbolic ./uniproxy-package/uniproxy.* ./unipack
ln --symbolic `pwd`/exps_pack/experiments ./experiments

PACKAGE_DIR="./unipack"
UNIPROXY_WRAPPER="./uniproxy-wrapper"
UNIPROXY_SUBWAY_WRAPPER="./subway-wrapper"

###############################################################################
#
#   Generate UNI proxy wrapper and supervisor config
#

cat <<EOF >>${UNIPROXY_WRAPPER}
#!/usr/bin/env bash

${PACKAGE_DIR}/uniproxy --port \${1:-80} &
UNIPROXY_PID="\$!"

function stop_uniproxy() {
        PIDS=\$(pstree -p \${UNIPROXY_PID} | grep -oe "([0-9]\\+)" | grep -oe "[0-9]\\+")
                kill -TERM \${PIDS}
            kill -TERM \${OUT_PID} \${ERR_PID}
}

trap "stop_uniproxy" EXIT TERM INT

wait \${UNIPROXY_PID}
EOF
chmod +x ${UNIPROXY_WRAPPER}


###############################################################################
#
#   Generate Subway config
#
cat <<EOF >>${UNIPROXY_SUBWAY_WRAPPER}
#!/usr/bin/env bash

$PACKAGE_DIR/uniproxy-subway &
SUBWAY_PID="\$!"

function stop_subway() {
        kill -TERM \${SUBWAY_PID}
}

trap "stop_subway" EXIT TERM INT

wait \${SUBWAY_PID}
EOF
chmod +x ${UNIPROXY_SUBWAY_WRAPPER}
