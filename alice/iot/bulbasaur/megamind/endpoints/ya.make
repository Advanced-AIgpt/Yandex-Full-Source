GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    const.go
    endpoint.go
    location.go
)

GO_TEST_SRCS(endpoint_test.go)

END()

RECURSE(gotest)
