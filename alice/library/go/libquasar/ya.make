GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    client.go
    config.go
    context.go
    errors.go
    interface.go
    metrics.go
    model.go
)

GO_TEST_SRCS(
    client_test.go
    config_test.go
    model_test.go
)

END()

RECURSE(gotest)
