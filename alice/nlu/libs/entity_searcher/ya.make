LIBRARY()

OWNER(
    yorky0
    g:alice_quality
)

PEERDIR(
    alice/nlu/granet/lib/sample
    alice/nlu/granet/lib/utils
    alice/nlu/libs/interval
    alice/nlu/libs/lemmatization
    alice/nlu/libs/tuple_like_type
    alice/nlu/proto/entities
    kernel/inflectorlib/phrase
    library/cpp/containers/comptrie
)

SRCS(
    sample_graph.cpp
    entity_searcher.cpp
    entity_searcher_builder.cpp
    entity_searcher_types.cpp
    entity_searcher_utils.cpp
)

END()

RECURSE_FOR_TESTS(ut)
