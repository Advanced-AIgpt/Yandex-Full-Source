GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    client.go
    device.go
    interface.go
    metrics.go
    testing.go
)

GO_TEST_SRCS(
    client_test.go
    device_test.go
)

END()

RECURSE(schema)

RECURSE_FOR_TESTS(gotest)
