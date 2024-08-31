GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    client.go
    const.go
    context.go
    interface.go
    metrics.go
    model.go
    option.go
    receivers.go
)

GO_TEST_SRCS(model_test.go)

END()

RECURSE(gotest)