LIBRARY()

OWNER(
    samoylovboris
    g:alice_quality
)

SRCS(
    granet.cpp
)

PEERDIR(
    alice/nlu/granet/lib/compiler
    alice/nlu/granet/lib/grammar
    alice/nlu/granet/lib/lang
    alice/nlu/granet/lib/parser
    alice/nlu/granet/lib/sample
    alice/nlu/granet/lib/test
    alice/nlu/granet/lib/user_entity
    alice/nlu/granet/lib/utils
    alice/nlu/granet/lib/vins_converter
)

END()

RECURSE_FOR_TESTS(ut)
