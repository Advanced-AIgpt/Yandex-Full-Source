#!/usr/bin/env bash

mkdir -p /logs/supervisor

# directory need for apphost ITS (backend patching)
mkdir -p .update_from_fs_tmp_dir
mkdir -p backends-after-its

# prepare TVM
xxd -l 16 -p /dev/urandom > ./token.txt

# make link for correct wrappers
ln --symbolic ./uniproxy-package/uniproxy.* ./unipack
ln --symbolic `pwd`/exps_pack/experiments ./experiments

PACKAGE_DIR="./unipack"
UNIPROXY_SUBWAY_WRAPPER="./subway-wrapper"


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
