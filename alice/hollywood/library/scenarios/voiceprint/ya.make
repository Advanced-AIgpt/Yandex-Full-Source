LIBRARY()

OWNER(
    isiv
    petrk
    klim-roma
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/registry
    alice/hollywood/library/scenarios/voiceprint/handles
    alice/hollywood/library/scenarios/voiceprint/nlg
    alice/hollywood/library/scenarios/voiceprint/proto
    alice/hollywood/library/scenarios/voiceprint/state_machine
    alice/hollywood/library/scenarios/voiceprint/util

    alice/library/proto
    apphost/lib/proto_answers
    search/begemot/rules/alice/response/proto
)

SRCS(
    GLOBAL voiceprint.cpp
)

END()

RECURSE_FOR_TESTS(
    it2
)
