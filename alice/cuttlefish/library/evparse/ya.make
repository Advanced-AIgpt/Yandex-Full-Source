LIBRARY()

OWNER(
    g:voicetech-infra
)

SRCS(
    evparse.cpp
)

PEERDIR(
    library/cpp/json
    alice/cuttlefish/library/protos
)

END()


RECURSE_FOR_TESTS(
    ut
    bench
)
