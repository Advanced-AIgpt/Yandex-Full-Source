LIBRARY()

OWNER(
    nkodosov
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/metronome/nlg
    alice/hollywood/library/scenarios/metronome/proto
)

SRCS(
    GLOBAL metronome.cpp
    metronome_helpers.cpp
)

END()

RECURSE_FOR_TESTS(
    it2
    ut
)
