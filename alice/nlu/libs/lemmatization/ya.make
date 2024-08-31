LIBRARY()

OWNER(
    samoylovboris
    vl-trifonov
    g:alice_quality
)

SRCS(
    lemmatize.cpp
)

PEERDIR(
    alice/nlu/libs/normalization
    dict/nerutil
    kernel/lemmer
    library/cpp/iterator
    library/cpp/langs
)

END()

RECURSE_FOR_TESTS(ut)
