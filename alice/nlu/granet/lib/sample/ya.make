LIBRARY()

OWNER(
    samoylovboris
    g:alice_quality
)

SRCS(
    entity.cpp
    markup.cpp
    sample.cpp
    sample_mock.cpp
    tag.cpp
)

PEERDIR(
    alice/nlu/granet/lib/lang
    alice/nlu/granet/lib/utils
    alice/nlu/libs/interval
    alice/nlu/libs/lemmatization
    alice/nlu/libs/normalization
    alice/nlu/libs/token_aligner
    alice/nlu/libs/tokenization
    alice/nlu/libs/tuple_like_type
    library/cpp/dbg_output
    library/cpp/iterator
    library/cpp/json
)

END()

RECURSE_FOR_TESTS(ut)
