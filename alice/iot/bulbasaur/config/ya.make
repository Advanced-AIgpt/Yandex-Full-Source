GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(config.go)

GO_TEST_SRCS(config_test.go)

END()

RECURSE(gotest)
