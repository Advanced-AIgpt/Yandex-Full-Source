GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    broker.go
    errors.go
    metrics.go
    payload.go
    queue.go
    retry.go
    task.go
    worker.go
    worker_options.go
)

GO_TEST_SRCS(
    payload_test.go
    retry_test.go
    worker_test.go
)

END()

RECURSE(pgbroker)

RECURSE_FOR_TESTS(gotest)
