LIBRARY()

OWNER(
    samoylovboris
    g:alice_quality
)

SRCS(
    vins_converter.cpp
    vins_config.cpp
)

PEERDIR(
    alice/nlu/granet/lib/compiler
    alice/nlu/granet/lib/sample
    alice/nlu/granet/lib/utils
    alice/nlu/libs/tokenization
    library/cpp/dbg_output
    library/cpp/json
)

END()
