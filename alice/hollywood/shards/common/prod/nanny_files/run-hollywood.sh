#!/usr/bin/env bash

if [[ "status" == "$1" ]]; then
    test "$(curl -s http://localhost:80/ping)" = "pong"
elif [[ "reopen_logs" == "$1" ]]; then
    test "$(curl -s http://localhost:80/reopen_logs)" = "OK"
else
    VAULT_VERSION="sec-01cxsqmp818f86wzv3rkshpctq"
    ./bin/hollywood/yav_wrapper -t YAV_TOKEN $VAULT_VERSION -- ./hollywood_server --current-dc $a_dc $@
fi
