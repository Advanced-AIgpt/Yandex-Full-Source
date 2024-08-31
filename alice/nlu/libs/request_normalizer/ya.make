LIBRARY()

OWNER(
    samoylovboris
    g:alice_quality
)

SRCS(
    request_normalizer.cpp
    request_tokenizer.cpp
)

PEERDIR(
    alice/nlu/libs/fst
    alice/nlu/libs/request_normalizer/data/denormalizer
    alice/nlu/libs/normalization
    contrib/libs/re2
    dict/dictutil
    library/cpp/langs
    library/cpp/resource
)

END()

RECURSE(
    java
)

RECURSE_FOR_TESTS(ut)
