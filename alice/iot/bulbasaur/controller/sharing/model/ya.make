GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(user.go)

GO_TEST_SRCS(user_test.go)

END()

RECURSE_FOR_TESTS(gotest)
