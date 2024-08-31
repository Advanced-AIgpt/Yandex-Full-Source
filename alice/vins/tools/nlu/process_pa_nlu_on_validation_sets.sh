#!/bin/bash

set -x

test_dir=apps/personal_assistant/personal_assistant/tests/validation_sets

EXPERIMENTS=disable_quasar_forbidden_intents,video_play,how_much,avia,translate,weather_precipitation,tv,tv_stream,quasar_tv,radio_play_in_quasar

BIN="./tools/nlu/nlu_tools process_nlu_on_dataset"

# downloading quasar_apr19_valid.tsv
# to download the dev version of the quasar dataset, please use resource 948395705
if [[ ! -f quasar_apr19_valid.tsv ]]; then
    sky get -w -N Backbone sbr:948382396
    tar -xzf resource.tar.gz
    rm resource.tar.gz
fi


$BIN --log-level ERROR personal_assistant classify \
    quasar_apr19_valid.tsv -o quasar_lsr.pkl \
    --app-info '{"app_id":"ru.yandex.quasar.services"}' --experiments=$EXPERIMENTS \
    --text-col text --intent-col intent --prev-intent-col prev_intent --device-state-col device_state \
    --apply-item-selection

test_name=search_release
test_file=$test_dir/$test_name.tsv
$BIN --log-level ERROR personal_assistant classify \
    $test_file -o $test_name.pkl --prev-intent-col prev_intent \
    --app-info='{"app_id": "winsearchbar", "platform": "windows"}' --experiments=$EXPERIMENTS


test_name=auto_release
test_file=$test_dir/$test_name.tsv
$BIN \
    --log-level ERROR \
    personal_assistant classify $test_file \
     -o $test_name.pkl \
    --app-info='{"app_id": "yandex.auto", "platform": "android"}'

test_name=navi_release
test_file=$test_dir/$test_name.tsv
$BIN \
    --log-level ERROR \
    personal_assistant classify $test_file \
    -o $test_name.pkl \
    --app-info '{"app_id":"ru.yandex.yandexnavi"}'
