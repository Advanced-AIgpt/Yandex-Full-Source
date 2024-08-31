LIBRARY()

OWNER(
    g:megamind
    alkapov
)

PEERDIR(
    alice/library/proto
    alice/library/json
    alice/megamind/library/util
    alice/megamind/protos/speechkit
    library/cpp/json
)

SRCS(
    constructor.cpp
)

END()
