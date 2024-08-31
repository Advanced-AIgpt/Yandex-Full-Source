LIBRARY()

OWNER(
    g:megamind
    alkapov
)

PEERDIR(
    alice/library/proto
    alice/megamind/library/common
    alice/megamind/library/models/interfaces
    alice/megamind/library/util
    alice/megamind/protos/common
    alice/megamind/protos/speechkit
)

SRCS(
    directives.cpp
)

END()

RECURSE(
    python
)

RECURSE_FOR_TESTS(ut)
