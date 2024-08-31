GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    const.go
    controller.go
    interface.go
)

GO_TEST_SRCS(const_test.go)

END()

RECURSE_FOR_TESTS(gotest)
