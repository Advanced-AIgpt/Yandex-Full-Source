LIBRARY()

OWNER(
    alkapov
    zubchick
    g:megamind
)

END()

RECURSE(
    common
    dev
    hamster
    production
    rc
)

RECURSE_FOR_TESTS(
    tests
    ut
)
