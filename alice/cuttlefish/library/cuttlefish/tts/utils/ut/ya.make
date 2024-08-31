UNITTEST_FOR(alice/cuttlefish/library/cuttlefish/tts/utils)

OWNER(g:voicetech-infra)

SRCS(
    utils_ut.cpp
)

PEERDIR(
    library/cpp/resource
)

RESOURCE(
    alice/uniproxy/library/settings/tts_development.json /python_uniproxy_tts_development.json
    alice/uniproxy/library/settings/tts_local.json /python_uniproxy_tts_local.json
    alice/uniproxy/library/settings/tts_rtc_alpha.json /python_uniproxy_tts_rtc_alpha.json
    alice/uniproxy/library/settings/tts_rtc_production.json /python_uniproxy_tts_rtc_production.json
    alice/uniproxy/library/settings/tts_testing.json /python_uniproxy_tts_testing.json
    alice/uniproxy/library/settings/tts_ycloud.json /python_uniproxy_tts_ycloud.json

    voicetech/library/uniproxy2/config/tts/tts_voices.json /uniproxy2_tts_voices.json
)

END()
