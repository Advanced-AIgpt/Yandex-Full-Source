GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    analytics.go
    apply_arguments.go
    apply_context.go
    apply_response_builder.go
    callback.go
    context.go
    continue_arguments.go
    continue_context.go
    continue_response_builder.go
    frame.go
    input.go
    logger.go
    response_builders.go
    run_context.go
    run_response_builder.go
    slots.go
)

GO_TEST_SRCS(frame_test.go)

END()

RECURSE(gotest)
