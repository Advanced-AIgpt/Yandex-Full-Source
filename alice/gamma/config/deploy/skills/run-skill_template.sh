#! /usr/bin/env bash

export SDK_ADDR=sdk.gamma-test.alice.yandex.net:8002

./guess-animal-game --port 8001 --sdk-addr ${SDK_ADDR} --log-json 1>/logs/skill.out 2>/logs/skill.err