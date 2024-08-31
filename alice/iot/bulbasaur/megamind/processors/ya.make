GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    action_processor.go
    directives.go
)

GO_TEST_SRCS(action_processor_test.go)

END()

RECURSE(gotest)
