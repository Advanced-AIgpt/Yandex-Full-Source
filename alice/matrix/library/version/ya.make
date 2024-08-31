LIBRARY()

OWNER(
    g:matrix
)

PEERDIR(
    build/scripts/c_templates
)

SRCS(
    version.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
