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
)

TEST_SRCS(
    test_event_patcher.py
    test_event_patcher_real.py
)

END()
