LIBRARY()

OWNER(
    g:alice
    lavv17
)

PEERDIR(
    alice/hollywood/library/tags/proto
    alice/library/proto_eval
)

SRCS(
    tags.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
