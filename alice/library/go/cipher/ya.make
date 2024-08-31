GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    cbc.go
    interface.go
)

GO_TEST_SRCS(cbc_test.go)

END()

RECURSE_FOR_TESTS(gotest)
