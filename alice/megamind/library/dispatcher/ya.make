LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/library/logger
    alice/library/metrics
    alice/megamind/library/apphost_request
    alice/megamind/library/config
    alice/megamind/library/globalctx
    alice/megamind/library/util
    library/cpp/threading/future
    apphost/api/service/cpp
)

SRCS(
    apphost_dispatcher.cpp
)

END()

RECURSE_FOR_TESTS(ut)
