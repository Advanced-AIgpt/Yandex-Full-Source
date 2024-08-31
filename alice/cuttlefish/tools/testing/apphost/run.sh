#!/bin/bash

# set -x

ulimit -n 4096

ARCADIA_ROOT=$(readlink -f $(dirname .)/../../../../..)
APPHOST_LAUNCHER_BIN=$ARCADIA_ROOT/apphost/tools/app_host_launcher/app_host_launcher

if ! [ -x $APPHOST_LAUNCHER_BIN ]; then
    echo "app_host_launcher binary $APPHOST_LAUNCHER_BIN doesn't exist, build it!"
    exit 1
fi

# testing: https://yav.yandex-team.ru/secret/sec-01dq7kz3k7e69t3xg0j7120rxv/explore/version/ver-01dq7kz3sa24yme9ey72r6zykm
# prod: https://yav.yandex-team.ru/secret/sec-01dq7ky9c7qm41fz04ve1f7njh/explore/version/ver-01dq7ky9hfhthxkrt0nh06wy55
if [ -n "$TVM_SECRET" ]; then
    if [ -z "$TVM_ID" ]; then
        echo "TVM_SECRET is set, but TVM_ID is not"
        exit 1
    fi
    echo "TVM_SECRET is set, tvm id is $TVM_ID"
    TVM_ARGS="--tvm-id $TVM_ID"
fi

rm -rf local_apphost_dir
$APPHOST_LAUNCHER_BIN setup --nora -P 40000 $TVM_ARGS -y -p local_apphost_dir/ arcadia -v VOICE --target-ctype hamster
