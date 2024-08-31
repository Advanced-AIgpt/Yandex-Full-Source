LIBRARY()

OWNER(
    g:voicetech-infra
)

SRCS(
    murmur.cpp
)

END()


RECURSE_FOR_TESTS(
    ut
)
