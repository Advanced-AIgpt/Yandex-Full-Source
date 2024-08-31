#!/bin/bash
cd /home/app || exit 125
if [[ "$ENV_TYPE" == "prod" ]] || [[ "$ENV_TYPE" == "hamster" ]] || [[ "$ENV_TYPE" == "test" ]] || [[ "$ENV_TYPE" == "dev" ]]; then
    java/run-java."$ENV_TYPE".sh
else
    echo "Error: unknown value for environment variable ENV_TYPE = '$ENV_TYPE'. Should be 'prod' or 'hamster' or 'test' or 'dev'"
    exit 125
fi
