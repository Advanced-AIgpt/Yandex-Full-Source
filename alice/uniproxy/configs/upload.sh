#!/bin/bash
THIS_DIR=$(dirname $(readlink -f $0))

if [ "$1" == "" ]; then
    echo "Upload configs into Sandbox. Usage: $0 {messenger|prod|quasar_beta}" >&2
    exit 0
fi

case $1 in
    "prod") RES_TYPE="VOICETECH_UNIPROXY_CONFIGS";;
    "quasar_beta") RES_TYPE="VOICETECH_UNIPROXY_QUASAR_CONFIGS";;
    "messenger") echo "TODO" >&2 && exit 1;;
    *) echo "Unknown environment \"$1\"" >&2 && exit 1;;
esac

ya upload -T=$RES_TYPE $1/configs
