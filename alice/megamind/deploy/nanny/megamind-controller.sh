#!/usr/bin/env bash

option="$1"
host="localhost"
port=80
yav_secret_id="sec-01cxsqmp818f86wzv3rkshpctq"

if [[ ${option} == "run" ]]; then
    ./bin/yav_wrapper ${yav_secret_id} -- ./bin/megamind_server --current-dc $a_dc "${@:2}"
elif [[ ${option} == "status" ]]; then
    test "$(curl -s http://${host}:${port}/ping)" = "pong"
elif [[ ${option} == "reload-logs" ]]; then
    curl -s http://${host}:${port}/reload_logs
else
    echo "unknown option"
    exit 1
fi
