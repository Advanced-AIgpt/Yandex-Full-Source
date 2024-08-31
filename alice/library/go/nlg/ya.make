GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(model.go)

GO_TEST_SRCS(model_test.go)

END()

RECURSE(gotest)
