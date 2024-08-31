LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/bass/libs/fetcher
    alice/megamind/library/apphost_request/protos
    alice/megamind/library/config/protos
    alice/megamind/library/globalctx
    alice/megamind/library/sources
    alice/megamind/library/util
    alice/library/censor/lib
    alice/library/logger
    alice/library/metrics
    library/cpp/cgiparam
    library/cpp/uri
)

SRCS(
    common.cpp
    requestctx.cpp
    rtlogtoken.cpp
    stage_timers.cpp
)

GENERATE_ENUM_SERIALIZATION(requestctx.h)
GENERATE_ENUM_SERIALIZATION(common.h)

END()

RECURSE_FOR_TESTS(ut)
