LIBRARY()

OWNER(
    samoylovboris
    g:alice_quality
)

SRCS(
    builder.cpp
)

PEERDIR(
    alice/nlu/granet/lib/utils
    alice/nlu/libs/interval
    alice/nlu/libs/tuple_like_type
    dict/dictutil
    library/cpp/iterator
    library/cpp/langs
)

END()

RECURSE_FOR_TESTS(ut)
