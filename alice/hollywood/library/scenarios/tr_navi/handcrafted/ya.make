LIBRARY()

OWNER(
    flimsywhimsy
    g:alice
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/bass_adapter
    alice/hollywood/library/metrics
    alice/hollywood/library/scenarios/tr_navi/handcrafted/nlg
)

SRCS(
    GLOBAL handcrafted.cpp
)

END()

RECURSE_FOR_TESTS(it)
