LIBRARY()

OWNER(
    vl-trifonov
    g:alice_quality
)

SRCS(
    aggregator.cpp
    config.cpp
    enum_generator.cpp
)

PEERDIR(
    alice/begemot/lib/feature_aggregator/proto
    alice/begemot/lib/fresh_options
    alice/begemot/lib/utils
    alice/protos/api/nlu
    alice/library/proto
    library/cpp/iterator
    library/cpp/langs
    search/begemot/core
    search/begemot/rules/alice/entities_collector/proto
    search/begemot/rules/alice/gc_dssm_classifier/proto
    search/begemot/rules/alice/multi_intent_classifier/proto
    search/begemot/rules/alice/parsed_frames/proto
    search/begemot/rules/porn_query/protos
    search/begemot/rules/wiz_detection/proto
)

END()

RECURSE_FOR_TESTS(ut)
