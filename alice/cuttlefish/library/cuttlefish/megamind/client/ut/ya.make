UNITTEST_FOR(alice/cuttlefish/library/cuttlefish/megamind/client)

OWNER(
    g:voicetech-infra
)

PEERDIR(
    library/cpp/http/server
    library/cpp/neh
    library/cpp/threading/future
)

SRCS(
    client_ut.cpp
)

END()
