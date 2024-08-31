LIBRARY()

OWNER(
    the0
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/http_proxy
    alice/library/network
    alice/megamind/protos/scenarios
    library/cpp/http/simple
    library/cpp/uri
    apphost/lib/common
)

SRCS(
    apphost_http_requester.cpp
    http_requester.cpp
)

END()

RECURSE_FOR_TESTS(ut)
