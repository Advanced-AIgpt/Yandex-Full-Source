LIBRARY()

OWNER(
    vl-trifonov
    g:alice_quality
)

PEERDIR(
    alice/nlu/libs/item_selector/interface
)

SRCS(
    composite.cpp
)

END()

RECURSE_FOR_TESTS(ut)
