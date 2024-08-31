GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    apply_response.go
    callbacks.go
    client_info.go
    const.go
    continue_response.go
    device_state.go
    directives.go
    dispatcher.go
    interface.go
    interfaces.go
    internet_connection.go
    iot_user_info.go
    led_animations.go
    logging.go
    output_response.go
    run_response.go
    semantic_frame.go
    user_info.go
)

GO_TEST_SRCS(
    internet_connection_test.go
    semantic_frame_test.go
)

END()

RECURSE_FOR_TESTS(gotest)
