GO_LIBRARY()

OWNER(
    g-kostin
    g:alice
)

PEERDIR(
    vendor/golang.org/x/xerrors
    alice/gamma/server/sdk/matcher/syntax
)

SRCS(
    machine.go
    matcher.go
)

GO_TEST_SRCS(
    machine_test.go
    matcher_test.go
)

END()

RECURSE(
    gotest
    syntax
)

RECURSE_FOR_TESTS(
    gotest
    syntax
)
