LIBRARY()

OWNER(
    alexanderplat
    g:alice_quality
    g:begemot
)

GENERATE_ENUM_SERIALIZATION(alice_response_polyglot_merger.h)

PEERDIR(
    alice/begemot/lib/frame_aggregator/proto
    alice/begemot/lib/polyglot_merge_response/proto

    search/begemot/rules/alice/response/proto
    search/begemot/rules/alice/polyglot_merge_response/proto
)

SRCS(
    alice_response_polyglot_merger.cpp
)

END()

RECURSE(
    proto
)

RECURSE_FOR_TESTS(
    ut
)
