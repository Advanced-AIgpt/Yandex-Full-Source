LIBRARY()

OWNER(
    dan-anastasev
    g:alice_quality
)

PEERDIR(
    alice/begemot/lib/entities_collector
    alice/begemot/lib/rule_utils
    alice/library/json
    alice/nlu/libs/embedder
    alice/nlu/libs/sample_features
    library/cpp/json
    library/cpp/langs
    library/cpp/scheme
    library/cpp/string_utils/base64
    search/begemot/core
    search/begemot/rules/alice/embeddings/proto
    search/begemot/rules/alice/normalizer/proto
    search/begemot/rules/dirty_lang/proto
    search/begemot/rules/entity_finder/proto
    search/begemot/rules/external_markup/proto
    search/begemot/rules/fst/proto
    search/begemot/rules/is_nav/proto
    search/begemot/rules/occurrences/custom_entities/rule/proto
)

SRCS(
    feature_extractor.cpp
)

END()
