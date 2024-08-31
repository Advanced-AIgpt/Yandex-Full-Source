#! /usr/bin/env bash

if [[ ${GAMMA_ENV_LABEL} == "production" ]]
then
  export SDK_ADDR=sdk.gamma.alice.yandex.net:8002
  export MUSIC_API=http://music-web-ext.music.yandex.net/internal-api/search
else
  export SDK_ADDR=sdk.gamma-test.alice.yandex.net:8002
  export MUSIC_API=http://api-pr-8.music.qa.yandex.net/search
fi

./song_search --port 8001 --sdk-addr ${SDK_ADDR} --music-api ${MUSIC_API} --log-json 1>>/logs/skill.out 2>>/logs/skill.err
