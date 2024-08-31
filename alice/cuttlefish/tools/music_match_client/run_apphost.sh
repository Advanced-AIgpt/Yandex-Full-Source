#!/bin/bash

# set -x

if [ "$TVM_SECRET" == "" ]; then
    echo 'TVM_SECRET not set'
    echo 'Go to https://yav.yandex-team.ru/secret/sec-01dq7kz3k7e69t3xg0j7120rxv/explore/versions and get it'
    exit 1
fi

rm -rf local_apphost_dir

../../../../apphost/tools/app_host_launcher/app_host_launcher setup --nora -P 20000 --tvm-id 2000743 -y -p local_apphost_dir/ arcadia -v VOICE --target-ctype test
