GO_LIBRARY()

OWNER(
    g-kostin
    g:alice
)

PEERDIR(
    alice/gamma/sdk/api
    alice/gamma/server/log
    alice/gamma/server/sdk/matcher
    vendor/google.golang.org/grpc
    vendor/github.com/hashicorp/golang-lru
)

SRCS(
    commands.go
    server.go
)

GO_TEST_SRCS(commands_test.go)

END()

RECURSE(
    gotest
    matcher
)

RECURSE_FOR_TESTS(
    gotest
    matcher
)
