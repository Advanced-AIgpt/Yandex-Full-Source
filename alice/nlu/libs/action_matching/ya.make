LIBRARY()

OWNER(
    volobuev
    g:alice_quality
)

PEERDIR(
    alice/begemot/lib/utils
    alice/megamind/protos/common
    alice/nlu/libs/binary_classifier
    alice/nlu/libs/embedder
    alice/nlu/libs/item_selector/default
    alice/nlu/libs/item_selector/interface
    alice/nlu/libs/item_selector/relevance_based
    library/cpp/json
    library/cpp/langs
    search/begemot/rules/granet/proto
    util
)

SRCS(
    action_matching.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
