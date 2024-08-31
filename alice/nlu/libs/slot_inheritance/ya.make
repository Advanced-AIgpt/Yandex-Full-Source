LIBRARY(slot_inheritance)

OWNER(
    alzaharov
    g:alice_quality
)

PEERDIR(
    alice/library/frame
    alice/nlu/granet/lib
)

SRCS(
    slot_inheritance.cpp
)

END()

RECURSE_FOR_TESTS(ut)
