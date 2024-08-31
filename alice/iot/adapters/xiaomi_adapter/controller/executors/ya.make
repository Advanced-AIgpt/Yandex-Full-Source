GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(executors.go)

GO_TEST_SRCS(executors_test.go)

END()

RECURSE(gotest)
