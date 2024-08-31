#!/bin/bash

# set -x

rm -rf all_results

for background in $(cat test_data/all_weather_backgrounds); do
    mkdir -p all_results/$background/short
    mkdir -p all_results/$background/long

    for mult in $(cat mults2); do
       echo "Gen now short $background $mult"
       ./tts-client \
            --need-tts-backend-timings false \
            --text "<speaker background='weather_backgrounds/$background'>Короткий текст" \
            --background-mult $mult \
            --result-path "result/tmp" || echo "$background $mutl" >> results/bad_list

        cp result/tmp/result.opus all_results/$background/short/$mult.opus

       echo "Gen now long $background $mult"
       ./tts-client \
            --need-tts-backend-timings false \
            --text "<speaker background='weather_backgrounds/$background'>Длинный текст чтобы был слышен почти весь фон. Погода, кстати, хорошая сегодня" \
            --background-mult $mult \
            --result-path "result/tmp" || echo "$background $mutl" >> results/bad_list

        cp result/tmp/result.opus all_results/$background/long/$mult.opus
    done
done
