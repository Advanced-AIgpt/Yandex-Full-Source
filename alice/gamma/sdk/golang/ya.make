GO_LIBRARY()

OWNER(
    g-kostin
    g:alice
)

PEERDIR(
    alice/gamma/sdk/api
    vendor/go.uber.org/zap
    vendor/golang.org/x/xerrors
    vendor/google.golang.org/grpc
)

SRCS(
    button.go
    card.go
    client.go
    log.go
    match.go
    sdk.go
)

GO_TEST_SRCS(
    client_test.go
    match_test.go
    sdk_test.go
)

END()

RECURSE(
    gotest
    testing
    dialog
)
