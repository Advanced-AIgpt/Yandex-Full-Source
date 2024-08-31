LIBRARY()

OWNER(
    g:megamind
    alexanderplat
)

PEERDIR(
    library/cpp/langs
)

SRCS(
    is_alice_worldwide_language.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
