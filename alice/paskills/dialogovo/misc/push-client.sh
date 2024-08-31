#!/usr/bin/env bash

cd /home/app || exit 125
if [[ -z "$ENV_TYPE" ]]; then
    echo "Error: environment variable ENV_TYPE is not set. Should be set to 'prod' or 'priemka' or 'test' or 'dev'"
    exit 125
elif [[ "$ENV_TYPE" == "prod" ]] || [[ "$ENV_TYPE" == "priemka" ]] || [[ "$ENV_TYPE" == "test" ]] || [[ "$ENV_TYPE" == "dev" ]]; then
    exec /usr/bin/push-client -f -c /usr/local/etc/push-client."$ENV_TYPE".yaml
else
    echo "Error: unknown value for environment variable ENV_TYPE = '$ENV_TYPE'. Should be 'prod' or 'priemka' or 'test' or 'dev'"
    exit 125
fi
