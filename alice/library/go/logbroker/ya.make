GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    reader.go
    wait.go
    write_pool.go
    writer.go
)

GO_TEST_SRCS(
    logbroker_test.go
    wait_test.go
)

END()

RECURSE(gotest)
