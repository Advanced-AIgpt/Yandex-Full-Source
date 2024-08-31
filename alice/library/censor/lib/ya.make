LIBRARY()

OWNER(g:alice_fun)

SRCS(
    censor.cpp
)

PEERDIR(
    alice/library/censor/protos

    contrib/libs/protobuf
)

END()

RECURSE_FOR_TESTS(ut)
