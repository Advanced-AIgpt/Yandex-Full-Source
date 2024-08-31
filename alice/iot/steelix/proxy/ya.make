GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    director.go
    proxy.go
    testing.go
    transport.go
)

GO_TEST_SRCS(
    director_test.go
    proxy_test.go
    transport_test.go
)

END()

RECURSE(gotest)
