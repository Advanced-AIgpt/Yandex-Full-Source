LIBRARY()

OWNER(
    the0
    g:alice
)

PEERDIR(
    alice/nlu/libs/request_normalizer
    contrib/libs/protobuf
    dict/dictutil
    library/cpp/containers/comptrie
    library/cpp/langs
    library/cpp/getopt
    library/cpp/packers
)

SRCS(
    occurrence_searcher.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
