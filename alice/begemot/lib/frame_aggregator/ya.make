LIBRARY()

OWNER(
    samoylovboris
    g:alice_quality
)

SRCS(
    aggregator.cpp
    config.cpp
)

PEERDIR(
    alice/begemot/lib/api/experiments
    alice/begemot/lib/frame_aggregator/proto
    alice/begemot/lib/fresh_options
    alice/begemot/lib/utils
    alice/library/frame
    alice/library/proto
    alice/megamind/protos/common
    library/cpp/langs
    search/begemot/core
    search/begemot/rules/alice/action_recognizer/proto
    search/begemot/rules/alice/binary_intent_classifier/proto
    search/begemot/rules/alice/entity_recognizer/proto
    search/begemot/rules/alice/fixlist/proto
    search/begemot/rules/alice/frame_filler/proto
    search/begemot/rules/alice/multi_intent_classifier/proto
    search/begemot/rules/alice/parsed_frames/proto
    search/begemot/rules/alice/tagger/proto
    search/begemot/rules/alice/trivial_tagger/proto
    search/begemot/rules/alice/word_nn/scenarios/proto
    search/begemot/rules/alice/word_nn/toloka/proto
    search/begemot/rules/granet/proto
    search/begemot/rules/wiz_detection/proto
)

GENERATE_ENUM_SERIALIZATION(config.h)

END()

RECURSE_FOR_TESTS(ut)
