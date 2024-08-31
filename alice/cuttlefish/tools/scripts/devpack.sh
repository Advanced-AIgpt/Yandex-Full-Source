#!/bin/bash
# This script builds needed tools and upload them into Sandbox as a single resource (.tar.gz)

ARCADIA_ROOT=$(readlink -f $(dirname $0)/../../../..)
APPHOST_ROOT=$ARCADIA_ROOT/apphost

CONTENT=(
    $APPHOST_ROOT/tools/event_log_dump/event_log_dump
    $APPHOST_ROOT/tools/grpc_client/grpc_client
)

function make() {
    ya make $1
    if [ $? != 0 ]; then
        exit -1
    fi
}

for ITEM in ${CONTENT[@]}; do
    make $(dirname $ITEM)
done

ya upload --tar --ttl=inf --description VOICE-APPHOST-DEVPACK `realpath ${CONTENT[@]}`
