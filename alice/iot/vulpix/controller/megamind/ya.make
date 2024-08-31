GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    cancel_processor.go
    connect_processors.go
    const.go
    controller.go
    directives.go
    failure_processors.go
    failure_processors_v2.go
    frame_action.go
    how_to_processor.go
    interface.go
    metrics.go
    model.go
    start_processors.go
    start_processors_v2.go
    success_processor.go
    success_processor_v2.go
)

GO_TEST_SRCS(const_test.go)

END()

RECURSE_FOR_TESTS(gotest)
