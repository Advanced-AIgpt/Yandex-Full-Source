GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(strings.go)

GO_TEST_SRCS(strings_test.go)

END()

RECURSE_FOR_TESTS(gotest)
