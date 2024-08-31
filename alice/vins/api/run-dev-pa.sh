#!/bin/sh

SCRIPT_DIR="$( realpath $(dirname "$0" ) )"

BIN="${SCRIPT_DIR}/pa/pa"

export YENV_TYPE=development
export VINS_MONGODB_URL=mongodb://localhost/vins_dm
export VINS_PRELOAD_APP=${1:-pa}
export VINS_ENABLE_BACKGROUND_UPDATES=1
export VINS_SPEECHKIT_LISTEN_BY_DEFAULT=1
export VINS_RESOURCES_PATH="${SCRIPT_DIR}/../resources"
export VINS_YANDEX_ALL_CAS_PATH="${SCRIPT_DIR}/YandexInternalRootCA.crt"
export VINS_TRANSITION_MODEL_LOGLEVEL=DEBUG
export VINS_DISABLE_REDIS_KNN_CACHE=1

curl "https://crls.yandex.net/allCAs.pem" > "${VINS_YANDEX_ALL_CAS_PATH}"

PORT=${2:-8000}

${BIN} -b [::]:${PORT} -w 1 --timeout 18000
