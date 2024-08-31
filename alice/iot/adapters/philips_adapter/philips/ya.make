GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    errors.go
    metrics.go
    model.go
    philips_client.go
    provider.go
)

GO_TEST_SRCS(model_test.go)

END()

RECURSE_FOR_TESTS(gotest)
