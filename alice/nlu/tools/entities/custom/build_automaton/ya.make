PROGRAM()

OWNER(
    the0
    g:alice
)

PEERDIR(
    alice/nlu/libs/occurrence_searcher
    alice/nlu/proto/entities
)

SRCS(
    build_automaton.cpp
)

END()
