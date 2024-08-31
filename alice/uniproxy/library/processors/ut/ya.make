PY3TEST()
OWNER(g:voicetech-infra)

PEERDIR(
    library/python/cityhash

    alice/cachalot/api/protos
    alice/megamind/protos/common
    alice/uniproxy/library/async_http_client
    alice/uniproxy/library/auth/mocks
    alice/uniproxy/library/backends_memcached
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/perf_tester
    alice/uniproxy/library/processors
    alice/uniproxy/library/testing
    alice/uniproxy/library/uaas
    alice/uniproxy/library/unisystem
    apphost/lib/proto_answers
)

TEST_SRCS(
    common.py
    test_asr.py
    test_asr_results_dropping.py
    test_base.py
    test_biometry.py
    test_log.py
    test_messenger.py
    test_messenger_processors.py
    test_processors.py
    test_save_vins_sessions_call.py
    test_set_state.py
    test_soundrecorder.py
    test_system.py
    test_tts.py
    test_uaas_url.py
    test_uniproxy2.py
    test_vins.py
    test_vins_apply_request_unistat.py
    test_vins_disregard_flags_per_request.py
    test_vins_text_input.py
    test_vins_voice_input_asr_partials.py
    test_vins_voice_input_biometry.py
    test_vins_voice_input_on_vins_response.py
    test_vins_voice_input_spotter_fail.py
    test_vins_voice_input_spotter_ok.py
    test_vins_voice_input_timings.py
    test_vins_context_load_response.py
)

RESOURCE(
    alice/uniproxy/experiments/experiments_rtc_production.json /experiments_rtc_production.json
    alice/uniproxy/experiments/vins_experiments.json /vins_experiments.json
)

END()
