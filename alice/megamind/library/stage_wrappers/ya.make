LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/megamind/protos/analytics
    alice/megamind/library/apphost_request
    alice/megamind/library/apphost_request/protos
    alice/megamind/library/response/proto
)

SRCS(
    postclassify_state.cpp
)

END()

RECURSE_FOR_TESTS(ut)
