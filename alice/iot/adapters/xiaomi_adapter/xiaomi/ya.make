GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    api_config.go
    client.go
)

GO_TEST_SRCS(client_test.go)

END()

RECURSE(
    iotapi
    miotspec
    model
    token
    userapi
)

RECURSE_FOR_TESTS(gotest)
