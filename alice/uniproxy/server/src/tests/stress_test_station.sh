for i in `seq 1 20 1000`
do
    TESTS_LIMIT=$(echo "$i*20" | bc)
    SIM_SESSIONS=$i
    echo `date '+%Y-%m-%d %H:%M:%S'` run $TESTS_LIMIT tests with $SIM_SESSIONS simultaneous sessions >>stress_log/stress_time.out
    python3 stress_test.py --tests-limit=$TESTS_LIMIT --simultaneous-sessions=$SIM_SESSIONS --test-session=stress_vins_voice_input_station.json --sess-result=stress_log/stress_run$i.out \
        --uniproxy=wss://voicestation.yandex.net/uni.ws \
        --auth-token=069b6659-984b-4c5f-880e-aaedcfd84102 \
        --audio-dir=stress_audio
    echo `date '+%Y-%m-%d %H:%M:%S'` finish tests >>stress_log/stress_time.out
    # sleep 20
done
