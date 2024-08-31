GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    analytics.go
    apply_response.go
    begemot_processor.go
    const.go
    controller.go
    directives.go
    frame_action.go
    frame_processor.go
    frame_router.go
    household_specify_processor.go
    interface.go
    metrics.go
    model.go
    output_response.go
    run_response.go
    sources.go
    time_specify_processor.go
)

GO_TEST_SRCS(model_test.go)

END()

RECURSE(
    arguments
    common
    endpoints
    frames
    intents
    processors
    sdk
)

RECURSE_FOR_TESTS(gotest)
