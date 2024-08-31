#!/usr/bin/env bash

cd /home/granet || exit 125
if [[ -z "$ENV_TYPE" ]]; then
    echo "Error: environment variable ENV_TYPE is not set. Should be set to 'prod' or 'test' or 'dev'"
    exit 125
elif [[ "$ENV_TYPE" == "prod" ]] || [[ "$ENV_TYPE" == "test" ]] || [[ "$ENV_TYPE" == "dev" ]]; then
    exec /usr/bin/push-client -f -c /home/granet/nanny/push_client."$ENV_TYPE".yaml
else
    echo "Error: unknown value for environment variable ENV_TYPE = '$ENV_TYPE'. Should be 'prod' or 'test' or 'dev'"
    exit 125
fi
