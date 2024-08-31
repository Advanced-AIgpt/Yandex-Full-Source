PY3TEST()
OWNER(g:voicetech-infra)

RESOURCE(
    alice/uniproxy/experiments/experiments_rtc_production.json /experiments_rtc_production.json
    alice/uniproxy/experiments/experiments_ycloud.json /experiments_ycloud.json
    alice/uniproxy/experiments/vins_experiments.json /vins_experiments.json
)

PEERDIR(
    alice/uniproxy/library/event_patcher
    alice/uniproxy/library/events
    alice/uniproxy/library/experiments
    alice/uniproxy/library/testing
    alice/uniproxy/library/utils
)

TEST_SRCS(
    common.py
    test_rtc_production.py
    test_dialogmaps.py
    test_voiceserv_2831.py
)

END()
