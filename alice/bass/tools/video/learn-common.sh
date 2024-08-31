#!/usr/bin/env bash

set -o errexit
set -o nounset


function try_upload_model {
    read -p "Upload model to Sandbox? ([y]es or [n]o): " upload
    upload=$(echo "$upload" | tr '[:upper:]' '[:lower:]')

    case "$upload" in
        y|yes) ya upload --ttl=inf "$MODEL"
               ;;
         n|no) ;;
            *) try_upload_model
               ;;
    esac
}

./"$GENERATE_TRAIN_DATA" <data.json --output-csv data.csv
./learn.py --input-csv data.csv --output-cbm "$MODEL"
try_upload_model
