#!/usr/bin/env bash

if [[ -z "$ENV_TYPE" ]]; then
    echo "Error: environment variable ENV_TYPE is not set. Should be set to 'production' or 'priemka' or 'test' or 'dev'"
    exit 125
elif [[ "$ENV_TYPE" == "production" ]] || [[ "$ENV_TYPE" == "priemka" ]] || [[ "$ENV_TYPE" == "load" ]] || [[ "$ENV_TYPE" == "test" ]] || [[ "$ENV_TYPE" == "dev" ]]; then
    TVM_TOKEN=$(cat ./token.txt)
    exec tvmtool -p "$TVM_PORT" -e -a "$TVM_TOKEN" -c /home/app/tvm/tvm."$ENV_TYPE".json
else
    echo "Error: unknown value for environment variable ENV_TYPE = '$ENV_TYPE'. Should be 'production' or 'priemka' or 'test' or 'dev'"
    exit 125
fi
