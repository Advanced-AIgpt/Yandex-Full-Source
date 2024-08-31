Y_BENCHMARK()

OWNER(
    g:voicetech-infra
)

SRCS(
    main.cpp
)

DATA(
    arcadia/alice/cuttlefish/library/evcheck/bench
)

PEERDIR(
    alice/cuttlefish/library/evcheck
    library/cpp/testing/unittest
)

END()
