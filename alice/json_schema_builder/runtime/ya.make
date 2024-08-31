LIBRARY()

OWNER(g:alice)

PEERDIR(
    contrib/libs/re2
    library/cpp/json
)

SRCS(
    runtime.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
