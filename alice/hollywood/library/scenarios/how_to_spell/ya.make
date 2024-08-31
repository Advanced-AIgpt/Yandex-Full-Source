LIBRARY()

OWNER(
    igor-darov
    g:alice
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/how_to_spell/nlg
    alice/hollywood/library/scenarios/how_to_spell/proto
)

SRCS(
    GLOBAL how_to_spell.cpp
    fast_data.cpp
)

END()

RECURSE_FOR_TESTS(
    it2
)
