LIBRARY()

OWNER(
    g:marketinalice
)

PEERDIR(
    alice/hollywood/library/scenarios/market/nlg
    alice/hollywood/library/scenarios/market/orders_status/proto

    alice/hollywood/library/bass_adapter
    alice/hollywood/library/framework
    alice/library/proto
)

SRCS(
    GLOBAL orders_status.cpp
)

END()

RECURSE_FOR_TESTS(
    it
)
