GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    controller.go
    directives.go
    interface.go
    testing.go
)

GO_TEST_SRCS(
    controller_test.go
    suite_test.go
)

END()

RECURSE(
    model
    gotest
)
