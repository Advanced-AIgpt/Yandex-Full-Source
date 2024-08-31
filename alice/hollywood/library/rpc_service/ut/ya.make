UNITTEST_FOR(alice/hollywood/library/rpc_service)

OWNER(
    g:megamind
)

PEERDIR(
    alice/hollywood/library/testing
    alice/library/unittest
    apphost/lib/service_testing
)

SRCS(
    rpc_request_ut.cpp
)

END()
