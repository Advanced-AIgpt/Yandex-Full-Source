#! /usr/bin/env bash

if [[ ${GAMMA_ENV_LABEL} == "production" ]]
then
  export SDK_ADDR=sdk.gamma.alice.yandex.net:8002
else
  export SDK_ADDR=sdk.gamma-test.alice.yandex.net:8002
fi

# https://clubs.at.yandex-team.ru/arcadia/20151
export GRPC_POLL_STRATEGY=epoll1

./guess_animal_game --port 8001 --sdk-addr ${SDK_ADDR} --log-json 1>>/logs/skill.out 2>>/logs/skill.err