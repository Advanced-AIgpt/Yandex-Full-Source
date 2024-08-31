LIBRARY()

OWNER(
    karina-usm
)

PEERDIR(
    alice/library/json
    alice/library/util
)

SRCS(
    billing.cpp
)

END()

RECURSE_FOR_TESTS(ut)
