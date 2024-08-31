GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    apply_arguments.go
    base_finish_processor.go
    callbacks.go
    cancel_processor.go
    const.go
    directives.go
    finish_processor.go
    finish_system_processor.go
    forget_devices_processor.go
    how_to_processor.go
    model.go
    nlg.go
    start_processor.go
    start_tuya_broadcast_processor.go
    state.go
    tuya_discovery_client.go
    utils.go
)

GO_TEST_SRCS(apply_arguments_test.go)

END()

RECURSE(gotest)
