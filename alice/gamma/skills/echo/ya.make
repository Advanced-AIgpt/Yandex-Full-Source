GO_PROGRAM()

OWNER(
    g-kostin
    g:alice
)

PEERDIR(
    alice/gamma/sdk/golang
    alice/gamma/sdk/golang/testing
)

SRCS(echo.go)

GO_TEST_SRCS(echo_test.go)

END()

RECURSE(gotest)
