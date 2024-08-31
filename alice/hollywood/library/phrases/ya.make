LIBRARY()

OWNER(
    g:alice
    lavv17
)

PEERDIR(
    alice/hollywood/library/phrases/proto
)

SRCS(
    phrases.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
