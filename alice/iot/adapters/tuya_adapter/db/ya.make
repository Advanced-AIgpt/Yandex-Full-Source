GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    client.go
    custom_control.go
    device_owner_cache.go
    interface.go
    metrics.go
    template.go
    testing.go
    user.go
)

GO_TEST_SRCS(
    client_test.go
    custom_control_test.go
    device_owner_cache_test.go
    user_test.go
)

END()

RECURSE(schema)

RECURSE_FOR_TESTS(gotest)
