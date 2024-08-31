GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    errors.go
    testing.go
)

GO_TEST_SRCS(errors_test.go)

END()

RECURSE_FOR_TESTS(gotest)
