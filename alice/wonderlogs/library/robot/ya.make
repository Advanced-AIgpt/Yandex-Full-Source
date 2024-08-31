LIBRARY()

OWNER(g:wonderlogs)

SRCS(
    account_type.cpp
)

PEERDIR(
    alice/wonderlogs/protos
)

END()

RECURSE_FOR_TESTS(ut)
