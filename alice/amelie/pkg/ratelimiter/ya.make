GO_LIBRARY()

OWNER(
    g:amelie
    alkapov
)

SRCS(
    limiter.go
)

GO_TEST_SRCS(
    limiter_test.go
)

END()

RECURSE_FOR_TESTS(gotest)
