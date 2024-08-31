GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    client_v2.go
    context.go
    error.go
    interface.go
    metrics.go
    model.go
)

GO_TEST_SRCS(model_test.go)

END()

RECURSE(gotest)
