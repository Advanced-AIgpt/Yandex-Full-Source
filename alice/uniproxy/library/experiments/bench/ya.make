PY3_PROGRAM()
OWNER(g:voicetech-infra)

PEERDIR(
    alice/uniproxy/library/experiments
    alice/uniproxy/library/events
)

RESOURCE(
    alice/uniproxy/experiments/experiments_rtc_production.json /experiments.json
    alice/uniproxy/experiments/vins_experiments.json /macros.json
    alice/uniproxy/library/experiments/bench/event_big.json /event_big.json
    alice/uniproxy/library/experiments/bench/event_little.json /event_little.json

)

PY_SRCS(
    MAIN main.py
)

END()
