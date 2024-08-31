GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    controller.go
    interface.go
    model.go
    testing.go
)

GO_TEST_SRCS(controller_test.go)

END()

RECURSE_FOR_TESTS(gotest)
