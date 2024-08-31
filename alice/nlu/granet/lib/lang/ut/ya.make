UNITTEST_FOR(alice/nlu/granet/lib/lang)

OWNER(
    samoylovboris
    g:alice_quality
)

SRCS(
    inflector_ut.cpp
    simple_tokenizer_ut.cpp
    synonym_generator_ut.cpp
)

PEERDIR(
    alice/nlu/libs/ut_utils
)

END()
