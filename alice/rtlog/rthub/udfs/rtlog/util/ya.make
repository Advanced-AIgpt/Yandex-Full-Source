LIBRARY()

OWNER(akhruslan g:megamind)

SRCS(
    util.cpp
)

PEERDIR(
    contrib/libs/re2
)

END()

RECURSE_FOR_TESTS(
    ut
)
