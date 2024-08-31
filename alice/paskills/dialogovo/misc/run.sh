#!/bin/bash
cd /home/app || exit 125
if [[ -z "$ENV_TYPE" ]]; then
    echo "Error: environment variable ENV_TYPE is not set. Should be set to 'prod' or 'priemka' or 'test' or 'dev'"
    exit 125
elif [[ "$ENV_TYPE" == "prod" ]]; then
    vmtouch -l -v -f /home/app/dialogovo.jar /home/app/lib /home/app/jdk/bin /usr/bin/push-client /usr/sbin/tvmtool &
    ./dialogovo_start.sh --environment=${ENV_TYPE}
elif [[ "$ENV_TYPE" == "priemka" ]] || [[ "$ENV_TYPE" == "test" ]] || [[ "$ENV_TYPE" == "dev" ]]; then
    vmtouch -l -v -f /home/app/dialogovo.jar /home/app/lib /home/app/jdk/bin /usr/bin/push-client /usr/sbin/tvmtool &
    ./dialogovo_start.sh --environment=${ENV_TYPE} --debug-port=5005 --jmx-port=1089
else
    echo "Error: unknown value for environment variable ENV_TYPE = '$ENV_TYPE'. Should be 'prod' or 'priemka' or 'test' or 'dev'"
    exit 125
fi
