LIBRARY()

OWNER(g:smarttv)

SRCS(
    matcher_util.cpp
    parser_util.cpp
    parsers.cpp
)

PEERDIR(
    alice/library/json
    alice/library/logger
    alice/library/proto
    alice/protos/data/video
    alice/protos/data/search_result
    alice/protos/data/scenario/objects
)

END()

RECURSE_FOR_TESTS(
    ut
)
