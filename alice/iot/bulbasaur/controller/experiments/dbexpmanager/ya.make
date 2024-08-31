GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(manager.go)

GO_TEST_SRCS(manager_test.go)

END()

RECURSE(gotest)
