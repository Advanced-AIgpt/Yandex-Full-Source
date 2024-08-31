#! /usr/bin/env bash

ENCODED_MONGO_PASSWORD=`python -c "import urllib; print urllib.quote_plus('$MONGO_PASSWORD')"`
export VINS_MONGODB_URL="mongodb://${MONGO_USER}:${ENCODED_MONGO_PASSWORD}@${MONGO_HOST}/${MONGO_DB}"
$RUN_COMMAND 1>> /logs/vins.out 2>> /logs/vins.err
