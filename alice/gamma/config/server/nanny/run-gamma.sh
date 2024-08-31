#! /usr/bin/env bash

if [[ ${GAMMA_ENV_LABEL} == "production" ]]
then
  CONFIG=production.yaml
else
  CONFIG=testing.yaml
fi

./gamma-server --port 80 --sdk-port 8002 --config ${CONFIG} 1>>/logs/server-log.out 2>>/logs/server-log.err
