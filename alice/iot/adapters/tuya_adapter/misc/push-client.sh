#!/usr/bin/env bash

if [ "${ENV_TYPE}" == "PRODUCTION" ]; then
    exec /usr/bin/push-client -f -c /usr/local/etc/push-client.conf.production
elif [ "${ENV_TYPE}" == "BETA" ]; then
    exec /usr/bin/push-client -f -c /usr/local/etc/push-client.conf.beta
else
    exec sleep 3153600000;  # 60*60*24*365*100
fi
