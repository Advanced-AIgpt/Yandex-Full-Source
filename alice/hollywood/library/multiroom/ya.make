LIBRARY()

OWNER(
    sparkle
    g:hollywood
    g:alice
)

PEERDIR(
    alice/hollywood/library/request
    alice/hollywood/library/response
    alice/library/logger
    alice/protos/data/device
)

SRCS(
    multiroom.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
