LIBRARY()

OWNER(
    g:marketinalice
)

PEERDIR(
    alice/hollywood/library/scenarios/market/common
    alice/hollywood/library/scenarios/market/how_much/nlg
    alice/hollywood/library/scenarios/market/how_much/proto

    alice/hollywood/library/bass_adapter
    alice/hollywood/library/framework
    alice/library/proto
    alice/nlu/libs/request_normalizer

    search/idl
)

SRCS(
    GLOBAL handles.cpp
    renderer.cpp
    run.cpp
    apply.cpp
    scenario.cpp
)

END()

RECURSE_FOR_TESTS(
    it
)
