#!/usr/bin/env bash

cd /home/app || exit 125
if [[ -z "$ENV_TYPE" ]]; then
    echo "Error: environment variable ENV_TYPE is not set. Should be set to 'prod' or 'test'"
    exit 125
elif [[ "$ENV_TYPE" == "prod" ]] || [[ "$ENV_TYPE" == "test" ]]; then
    exec tvmtool -p "$TVM_PORT" -e -a "$TVM_TOKEN" -c /usr/local/etc/tvm."$ENV_TYPE".json
else
    echo "Error: unknown value for environment variable ENV_TYPE = '$ENV_TYPE'. Should be 'prod' or 'test'"
    exit 125
fi