GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(matcher.go)

GO_TEST_SRCS(matcher_test.go)

END()

RECURSE(gotest)
