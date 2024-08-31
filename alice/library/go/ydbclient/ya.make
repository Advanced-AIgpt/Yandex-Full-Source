GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    client.go
    metrics.go
    row.go
    stub.go
    transaction.go
)

GO_TEST_SRCS(
    client_test.go
    transaction_test.go
)

END()

RECURSE(gotest)
