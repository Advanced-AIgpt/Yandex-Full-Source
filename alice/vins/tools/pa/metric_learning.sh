#!/usr/bin/env bash

extra_args="$*"

load_custom_entities_cmd=""
if [[ $extra_args != *"--load-custom-entities"* ]]; then
    compile_app_model="tools/nlu/nlu_tools compile_app_model \
    --app personal_assistant \
    --update-config \
    --fst-only"
    load_custom_entities_cmd="$compile_app_model"
fi

train_metric_cmd="tools/train/train_tools train_metric_learning \
--load-custom-entities \
--custom-intents \
personal_assistant/config/scenarios/scenarios.mlconfig.json \
personal_assistant/config/handcrafted/handcrafted.mlconfig.json \
personal_assistant/config/stroka/stroka.mlconfig.json \
personal_assistant/config/navi/navi.mlconfig.json \
$extra_args"

if [[ -z "$load_custom_entities_cmd" ]]; then
    cmd="$train_metric_cmd"
else
    cmd="$load_custom_entities_cmd && $train_metric_cmd"
fi

eval "$cmd"
