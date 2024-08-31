Y_BENCHMARK()
OWNER(g:voicetech-infra)

SRCS(
    main.cpp
)

RESOURCE(
    experiments.json /experiments.json
    macros.json /macros.json
    event_big.json /event_big.json
    event_small.json /event_small.json
    event_synchronize_state.json /event_synchronize_state.json
)

PEERDIR(
    alice/cuttlefish/library/experiments
)

END()
