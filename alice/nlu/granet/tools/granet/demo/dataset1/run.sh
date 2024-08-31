#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARCADIA="${SCRIPT_DIR%/alice/nlu*}"
cd "$SCRIPT_DIR"

mkdir -p dst

dataset_help() {
    echo "============ dataset_help ============"
    ../../granet dataset --help
}

dataset_create_help() {
    echo "============ dataset_create_help ============"
    ../../granet dataset create --help
}

dataset_create() {
    echo "============ dataset_create ============"
    ../../granet dataset create \
        -i src.tsv \
        -o dst/create.tsv \
        --normalize \
        --lang ru
}

dataset_update_entities_help() {
    echo "============ dataset_update_entities_help ============"
    ../../granet dataset update-entities --help
}

dataset_update_entities() {
    echo "============ dataset_update_entities ============"
    ../../granet dataset update-entities \
        -i src.tsv \
        -o dst/entities.tsv \
        --lang ru
}

dataset_two_step_creation() {
    echo "============ dataset_two_step_creation ============"
    ../../granet dataset create \
        -i src.tsv \
        -o dst/dataset.tsv \
        --normalize \
        --lang ru \
        --no-entities

    ../../granet dataset update-entities \
        -i dst/dataset.tsv \
        --lang ru \
        --missing
}

dataset_select_help() {
    echo "============ dataset_select_help ============"
    ../../granet dataset select --help
}

dataset_select() {
    echo "============ dataset_select ============"
    ../../granet dataset select \
        -i dst/dataset.tsv \
        -p dst/select.tsv \
        -g "$ARCADIA/alice/nlu/data/ru/granet/main.grnt" \
        --keep-extra yes \
        --form alice.random_number \
        --lang ru
}

dataset_test_help() {
    echo "============ dataset_test_help ============"
    ../../granet dataset test --help
}

dataset_test() {
    echo "============ dataset_test ============"
    ../../granet dataset test \
        --positive dst/dataset.tsv \
        --false-positive dst/test_fp.tsv \
        --false-negative dst/test_fn.tsv \
        --true-positive dst/test_tp.tsv \
        --true-negative dst/test_tn.tsv \
        --tagger-mismatch dst/test_tagger.tsv \
        --report dst/test_report \
        --keep-weight yes \
        --keep-extra yes \
        --grammar "$ARCADIA/alice/nlu/data/ru/granet/main.grnt" \
        --form alice.random_number \
        --lang ru
}

# Begemot url customization
# export GRANET_BEGEMOT=cuda-sge08h.dev.voicetech.yandex.net:31917

dataset_help

dataset_create_help
dataset_create

dataset_update_entities_help
dataset_update_entities

dataset_two_step_creation

dataset_select_help
dataset_select

dataset_test_help
dataset_test
