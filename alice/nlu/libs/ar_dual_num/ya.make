LIBRARY()

OWNER(
    cardstell
    g:alice_quality
)

SRCS(
    ar_dual_num.cpp
)

PEERDIR(
    alice/nlu/libs/normalization
    alice/nlu/proto/entities
)

END()

RECURSE_FOR_TESTS(ut)
