UNITTEST_FOR(alice/cuttlefish/library/cuttlefish/megamind)

OWNER(
    g:voicetech-infra
)

PEERDIR(
    alice/megamind/protos/speechkit
    library/cpp/http/server
    library/cpp/neh
    library/cpp/threading/future
)

SRCS(
    utils_ut.cpp
)

END()
