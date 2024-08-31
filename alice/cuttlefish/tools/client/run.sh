#!/bin/bash

CLIENT_BIN=$(dirname $(readlink -f $0))/client
HOST=uniproxy2-trunk-copy-wsnk-2.sas.yp-c.yandex.net
SUFFIX=$1
FILTER=$2

$CLIENT_BIN --verbose --address $HOST:3001/synchronize_state_2$SUFFIX --use-local 40001 \
    --srcrwr "APIKEYS" "http://apikeys-ipv6.yandex.net:8666/api" \
    $FILTER

