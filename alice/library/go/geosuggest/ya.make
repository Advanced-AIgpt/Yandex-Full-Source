GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    client.go
    context.go
    interface.go
    metrics.go
    model.go
)

GO_TEST_SRCS(model_test.go)

END()

RECURSE_FOR_TESTS(gotest)
