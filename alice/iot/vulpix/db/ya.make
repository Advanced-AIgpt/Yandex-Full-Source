GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    client.go
    connecting_device_type.go
    device_state.go
    interface.go
    metrics.go
    testing.go
)

GO_TEST_SRCS(
    client_test.go
    connecting_device_type_test.go
    device_state_test.go
)

END()

RECURSE(schema)

RECURSE_FOR_TESTS(gotest)
