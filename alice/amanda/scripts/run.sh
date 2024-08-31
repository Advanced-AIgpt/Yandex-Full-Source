#!/bin/bash

# script to run Amanda Dev bot


ARCADIA_ROOT="${ARCADIA_ROOT:-.}"
TVMTOOL_LOCAL_AUTHTOKEN=$(python -c "import uuid; print(str(uuid.uuid4()).replace('-', ''))")
TVMTOOL_LOCAL_PORT=8079


ya make -r "${ARCADIA_ROOT}/alice/amanda/cmd/amanda"


mkdir -p /tmp/amanda_tvm_cache
if [ ! -f "/tmp/amanda_tvm_cache/tvmtool" ]; then
    curl 'https://proxy.sandbox.yandex-team.ru/last/TVM_TOOL_BINARY?arch=linux&state=READY' --output /tmp/amanda_tvm_cache/tvmtool
    chmod +x /tmp/amanda_tvm_cache/tvmtool
fi

if [[ -z $AMANDA_TGBOT_TOKEN ]]; then
    AMANDA_TGBOT_TOKEN=$(ya vault get version ver-01fz88h5hcw6w1ngw36yn8dae5 -o "telegram.dev.token")
    echo "Using default dev bot https://t.me/AmandaJohnsonDevBot. Set AMANDA_TGBOT_TOKEN evn to use custom bot"
fi

trap "exit" INT TERM ERR
trap "kill 0" EXIT


ZORA_TVM=$(ya vault get version ver-01fz88h5hcw6w1ngw36yn8dae5 -o "zora.tvm") \
/tmp/amanda_tvm_cache/tvmtool \
    --config "${ARCADIA_ROOT}/alice/amanda/scripts/tvm_amanda.conf" \
    --cache-dir /tmp/amanda_tvm_cache \
    --port "$TVMTOOL_LOCAL_PORT" \
    --auth "$TVMTOOL_LOCAL_AUTHTOKEN" &


MONGO_PASSWORD=$(ya vault get version ver-01fz88h5hcw6w1ngw36yn8dae5 -o "mongo.password") \
PASSPORT_CLIENT_ID=$(ya vault get version ver-01fz88h5hcw6w1ngw36yn8dae5 -o "passport.client_id") \
PASSPORT_CLIENT_SECRET=$(ya vault get version ver-01fz88h5hcw6w1ngw36yn8dae5 -o "passport.client_secret") \
S3_ACCESS_KEY=$(ya vault get version ver-01fz88h5hcw6w1ngw36yn8dae5 -o "s3.access_key") \
S3_SECRET_KEY=$(ya vault get version ver-01fz88h5hcw6w1ngw36yn8dae5 -o "s3.secret_key") \
STAFF_OAUTH_TOKEN=$(ya vault get version ver-01fz88h5hcw6w1ngw36yn8dae5 -o "staff.oauth_token") \
TELEGRAM_TOKEN=$AMANDA_TGBOT_TOKEN \
DEPLOY_TVM_TOOL_URL="http://localhost:$TVMTOOL_LOCAL_PORT" \
TVMTOOL_LOCAL_AUTHTOKEN="$TVMTOOL_LOCAL_AUTHTOKEN" \
"${ARCADIA_ROOT}/alice/amanda/cmd/amanda/amanda" -c "${ARCADIA_ROOT}/alice/amanda/configs/dev.yaml" &


wait
