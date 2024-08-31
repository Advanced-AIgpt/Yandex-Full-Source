#!/bin/bash

# set -x

rm -rf results/bad_list
# rm -rf results

for lang in $(cat langs); do
    for voice in $(cat voices); do
       echo "Test now $lang $voice"
       ./tts-client \
            --replace-shitova-with-shitova-gpu false \
            --need-tts-backend-timings false \
            --enable-get-from-cache false \
            --enable-save-to-cache false \
            --text "1 2 3 4 5 6" \
            --voice-lang "$lang" \
            --voice-voice "$voice" \
            --result-path "results/$lang/$voice" || echo "$lang $voice" >> results/bad_list
    done
done
