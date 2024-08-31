LIBRARY()

OWNER(
    deemonasd
    g:alice_quality
)

SRCS(
    compression.cpp
)

END()

RECURSE_FOR_TESTS(ut)
