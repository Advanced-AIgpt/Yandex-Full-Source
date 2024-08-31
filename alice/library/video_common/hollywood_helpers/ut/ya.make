UNITTEST_FOR(alice/library/video_common/hollywood_helpers)

OWNER(g:alice)

PEERDIR(
    apphost/lib/service_testing
    alice/library/unittest
    alice/hollywood/protos
    alice/hollywood/library/base_scenario
    alice/hollywood/library/framework/core
    alice/hollywood/library/http_proxy
    alice/hollywood/library/response
)

SRCS(
    utils_ut.cpp
)

END()
