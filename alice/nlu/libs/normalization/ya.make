LIBRARY()

OWNER(
    cardstell
    samoylovboris
    vl-trifonov
    g:alice_quality
)

SRCS(
    ar_normalize.cpp
    normalize.cpp
)

PEERDIR(
    dict/dictutil
    library/cpp/langs
    library/cpp/unicode/normalization
    contrib/libs/re2
)

END()

RECURSE_FOR_TESTS(
    ut
)
