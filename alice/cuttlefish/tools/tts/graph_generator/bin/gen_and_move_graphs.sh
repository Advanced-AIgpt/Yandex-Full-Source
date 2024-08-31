#!/bin/bash

# set -x

./graph-generator
cp tts_backend.json s3_audio.json tts.json tts_generate.json ../../../../../../apphost/conf/verticals/VOICE
rm -rf tts_backend.json s3_audio.json tts.json tts_generate.json
