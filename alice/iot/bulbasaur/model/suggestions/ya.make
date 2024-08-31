GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(suggestions.go)

GO_XTEST_SRCS(suggestions_test.go)

END()

RECURSE(gotest)
