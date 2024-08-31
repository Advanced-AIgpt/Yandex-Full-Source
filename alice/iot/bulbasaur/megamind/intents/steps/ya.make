GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    actions_processor.go
    actions_result_callback_processor.go
    apply_arguments.go
    callbacks.go
    sideeffects.go
)

GO_TEST_SRCS(actions_processor_test.go)

END()

RECURSE(gotest)
