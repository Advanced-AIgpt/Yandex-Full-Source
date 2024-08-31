#!/usr/bin/env bash

set -ex

cmd_start() {
    "./bin/hollywood/yav_wrapper" -t BASS_AUTH_TOKEN 'sec-01cxsqmp818f86wzv3rkshpctq' -- \
        ./bin/bass/bass_server \
        -V "EventLogFile=/logs/current-bass-rtlog" \
        -V "ENV_GEOBASE_PATH=./common_resources/geodata6.bin" \
        --logdir "./logs/" \
        --port '100' \
        --lock-geobase \
        --current-dc $a_dc \
        ${@:1}

    exit $?
}

cmd_status() {
    nc -z localhost 100 || exit 33
    exit $?
}

cmd_reload_logs() {
    pkill -HUP bass_server
    exit $?
}

while [[ -n "$1" ]]; do
    case "$1" in
    start)
        cmd_start ${@:2}
        ;;
    status)
        cmd_status
        ;;
    reload-logs)
        cmd_reload_logs
        ;;
    esac
    shift
done

echo "Unknown action: '$1'" 1>&2

exit 1
