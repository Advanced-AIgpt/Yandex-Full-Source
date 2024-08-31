LIBRARY()

OWNER(
    sparkle
    g:alice
)

SRCS(
    s3_animations.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
