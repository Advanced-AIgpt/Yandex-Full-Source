GO_LIBRARY()

OWNER(
    g-kostin
    g:alice
)

PEERDIR(
    alice/gamma/sdk/api
    vendor/golang.org/x/xerrors
)

SRCS(
    button.go
    card.go
    request.go
)

GO_TEST_SRCS(
    api_test.go
    request_test.go
)

END()

RECURSE(
    admin
    gotest
)
