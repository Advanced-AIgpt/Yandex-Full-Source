GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    const.go
    context.go
    controller.go
    events.go
    interface.go
    metrics.go
    model.go
)

GO_TEST_SRCS(controller_test.go)

END()

RECURSE_FOR_TESTS(gotest)
