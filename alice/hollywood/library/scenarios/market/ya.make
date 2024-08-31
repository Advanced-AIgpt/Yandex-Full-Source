LIBRARY()

OWNER(
    g:marketinalice
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/market/common
    alice/hollywood/library/scenarios/market/how_much
    alice/hollywood/library/scenarios/market/orders_status
)

END()

RECURSE_FOR_TESTS(
    common
    how_much
    orders_status
)
