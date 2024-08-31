LIBRARY()

OWNER(
    andrewgark
    g:megamind
    g:alice_quality
)

PEERDIR(
    alice/begemot/lib/utils
    alice/library/find_poi
    alice/library/frame
    alice/library/json
    alice/library/music
    alice/library/request
    alice/library/search
    alice/library/video_common
    alice/megamind/protos/common
    alice/nlu/granet/lib/sample
    search/begemot/rules/alice/entities_collector/proto
    search/begemot/rules/alice/tagger/proto
    search/begemot/rules/granet/proto
    search/begemot/rules/occurrences/custom_entities/rule/proto
)

SRCS(
    collected_entities.cpp
    tagger_slot_matching.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
    ut/data
)
