LIBRARY()

OWNER(d-dima)

SRCS(
    search_result_parser.cpp
)

PEERDIR(
    alice/library/json
    alice/library/logger
    alice/library/proto
    alice/megamind/protos/scenarios
    alice/protos/data/scenario/objects
    alice/protos/data/scenario/video
)

END()

RECURSE_FOR_TESTS(
    ut
)

RECURSE(video)
