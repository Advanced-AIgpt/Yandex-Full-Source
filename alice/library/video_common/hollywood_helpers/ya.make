OWNER(
    g:alice
)

LIBRARY()

SRCS(
    proxy_request_builder.cpp
    util.cpp
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/framework/core
    alice/hollywood/library/http_proxy
    alice/hollywood/library/response
    alice/library/video_common
)

END()

RECURSE_FOR_TESTS(ut)
