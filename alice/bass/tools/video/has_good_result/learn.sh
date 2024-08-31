#!/usr/bin/env bash

set -o errexit
set -o nounset

export MODEL="has_good_result.cbm"
export GENERATE_TRAIN_DATA="generate_train_data/generate_train_data"

case $# in
    0) ;;
    *) echo "This scripts learns a catboost binary classifier for the has-good-result scenario."
       echo "The model will be written to $MODEL."
       echo
       echo "Usage: $0" 2>&1
       exit 1
       ;;
esac

cd $(dirname "$0")
../learn-common.sh
