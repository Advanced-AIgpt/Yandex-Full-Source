LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/library/analytics/common
    alice/library/censor/lib
    alice/library/metrics
    alice/library/logger
    alice/megamind/library/analytics
    alice/megamind/library/apphost_request
    alice/megamind/library/apphost_request/protos
    alice/megamind/library/globalctx
    alice/megamind/library/requestctx
    alice/megamind/library/response
    alice/megamind/library/response_meta
    alice/megamind/library/sources
    alice/megamind/library/walker
    alice/megamind/library/util
    alice/wonderlogs/sdk/utils
    library/cpp/http/misc
    library/cpp/json
    library/cpp/protobuf/json
)

SRCS(
    analytics_logs_context_builder.cpp
    http_response.cpp
    logs_util.cpp
    sensors.cpp
)

END()

RECURSE_FOR_TESTS(ut)
