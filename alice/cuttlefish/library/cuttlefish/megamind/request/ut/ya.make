UNITTEST_FOR(alice/cuttlefish/library/cuttlefish/megamind/request)

OWNER(
    g:voicetech-infra
)

PEERDIR(
    apphost/lib/service_testing
    library/cpp/http/server
    library/cpp/neh
    library/cpp/threading/future
)

SRCS(
    request_builder_ut.cpp
)

END()
