GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    handlers.go
    server.go
)

GO_TEST_SRCS(worker_test.go)

END()

RECURSE(gotest)
