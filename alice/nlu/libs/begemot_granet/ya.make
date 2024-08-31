LIBRARY()

OWNER(
    samoylovboris
    g:alice_quality
)

SRCS(
    data_loader.cpp
)

PEERDIR(
    alice/nlu/granet/lib/compiler
    alice/nlu/granet/lib/grammar
    alice/nlu/granet/lib/utils
    search/begemot/core
)

END()
