LIBRARY()

OWNER(
    igor-darov
    g:alice_quality
)

PEERDIR(
    alice/library/iot
    alice/megamind/protos/common
    alice/nlu/granet/lib/sample
    alice/nlu/libs/entity_searcher
    alice/nlu/libs/lemmatization
    alice/nlu/libs/normalization
    alice/nlu/libs/occurrence_searcher
    alice/nlu/libs/request_normalizer
    alice/nlu/libs/tokenization
    kernel/lemmer/core
    library/cpp/iterator
    library/cpp/regex/pcre
    library/cpp/scheme
)

SRCS(
    custom_entities.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
