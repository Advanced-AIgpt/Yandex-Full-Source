PY3_LIBRARY()

OWNER(
    g:matrix
    g:voicetech-infra
)

PY_SRCS(
    __init__.py
    spotter_maps.py
)

RESOURCE(
    development.json  /development.json
    lang_maps.json  /lang_maps.json
    rtc_alpha.json  /rtc_alpha.json
    testing.json  /testing.json
    tts_testing.json  /tts_testing.json
    rtc_production.json /rtc_production.json
    rtc_delivery_production.json /rtc_delivery_production.json
    rtc_delivery_alpha.json /rtc_delivery_alpha.json
    spotter_topics.json /spotter_topics.json
    notificator.json    /notificator.json
    topic_maps.json /topic_maps.json
    tts_development.json  /tts_development.json
    tts_rtc_alpha.json  /tts_rtc_alpha.json
    tts_rtc_production.json /tts_rtc_production.json
    tts_ycloud.json /tts_ycloud.json
    ycloud.json /ycloud.json
    local.json /local.json
    tts_local.json /tts_local.json
)

PEERDIR(
    alice/uniproxy/library/logging
    contrib/python/frozendict
    library/python/resource
)

END()

RECURSE_FOR_TESTS(ut)
