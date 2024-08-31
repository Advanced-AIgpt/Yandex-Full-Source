LIBRARY()

OWNER(
    g:alice_quality
    smirnovpavel
)

PEERDIR(
    alice/nlu/libs/annotator
    alice/nlu/libs/fst
    library/cpp/containers/comptrie
    library/cpp/resource
)

SRCS(
    dictionary.cpp
    digital.cpp
    fst.cpp
    type_parser.cpp
    types.cpp
    union.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
