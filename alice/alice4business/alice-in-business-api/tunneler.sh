#!/bin/bash
set -e
if [ -z $3 ]; then
    echo "Usage: ./tunneler.sh <tunneler_host> <local_port> <project_id>"
    exit -1
fi
echo "Asking $1 for instance list"
IFS=',' INST=($(curl "https://$1/api/v1/instances?format=ipv6" | tr -d [] | awk '{FS=","; print $0 }' | tr -d '"'))
HOST=${INST[0]}
PORT=${INST[1]}
echo "Connecting to ${HOST}:${PORT}"
echo "Open https://${USER}-${3}-xproducts.ldev.yandex.ru"
ssh -o UserKnownHostsFile=/dev/null -o LogLevel=ERROR -o StrictHostKeyChecking=no -N -R "$3:localhost:$2" -p $PORT $HOST
