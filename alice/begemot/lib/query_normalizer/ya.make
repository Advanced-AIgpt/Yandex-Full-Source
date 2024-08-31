LIBRARY()

OWNER(
    tolyandex
    g:alice_quality
    g:begemot
)

PEERDIR(
    alice/nlu/libs/token_interval_inflector
    alice/nlu/libs/interval
    contrib/libs/re2
)

SRCS(
    query_normalizer.cpp
)

END()

RECURSE_FOR_TESTS(ut)
