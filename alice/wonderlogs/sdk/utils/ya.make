LIBRARY()

OWNER(g:wonderlogs)

SRCS(
    speechkit_utils.cpp
)

PEERDIR(
    alice/library/client
    alice/library/experiments
    alice/library/json
    alice/library/proto

    alice/megamind/library/response

    alice/megamind/protos/common
    alice/megamind/protos/speechkit
    alice/megamind/protos/analytics
)

END()

RECURSE_FOR_TESTS(
    ut
)
