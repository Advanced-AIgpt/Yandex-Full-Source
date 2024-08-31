UNITTEST_FOR(alice/nlu/granet/lib/compiler)

OWNER(
    samoylovboris
    g:alice_quality
)

SRCS(
    expression_tree_builder_ut.cpp
    preprocessor_ut.cpp
)

PEERDIR(
    alice/nlu/libs/ut_utils
)

END()
