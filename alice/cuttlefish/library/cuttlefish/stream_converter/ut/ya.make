UNITTEST_FOR(alice/cuttlefish/library/cuttlefish/stream_converter)

OWNER(g:voicetech-infra)

SRCS(
    asr_recognize_ut.cpp
    asr_result_ut.cpp
    biometry_ut.cpp
    matched_user_event_handler_ut.cpp
    music_match_request_ut.cpp
    music_match_response_ut.cpp
    proto_to_ws_stream_ut.cpp
    smart_activation_ut.cpp
    support_functions_ut.cpp
    tts_generate_response_ut.cpp
    tts_generate_ut.cpp
)

PEERDIR(
    apphost/lib/service_testing
    alice/library/json
    library/cpp/json
)

END()
