GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    pg_broker.go
    pg_broker_with_metrics.go
)

GO_XTEST_SRCS(example_test.go)

END()

RECURSE(gotest)
