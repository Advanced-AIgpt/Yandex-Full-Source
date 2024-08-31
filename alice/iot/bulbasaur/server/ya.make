GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    api_handlers.go
    apphost_handlers.go
    apphost_megamind_handlers.go
    callback_errors.go
    callback_handlers.go
    common.go
    dialogs_handlers.go
    doc.go
    event_handlers.go
    megamind.go
    megamind_handlers.go
    mobile_handlers.go
    push_errors.go
    push_handlers.go
    server.go
    takeout_handlers.go
    testing.go
    time_machine_handlers.go
    user_handlers.go
    widget_handlers.go
)

GO_TEST_SRCS(
    action_controller_test.go
    api_handlers_test.go
    callback_handlers_test.go
    common_test.go
    dialogs_handlers_test.go
    event_handlers_test.go
    megamind_handlers_test.go
    mobile_handlers_test.go
    push_handlers_test.go
    router_test.go
    server_test.go
    takeout_handlers_test.go
    time_machine_handlers_test.go
    user_handlers_test.go
    widget_handlers_test.go
)

END()

RECURSE(
    api
    apierrors
    gotest
    services
    swagger
)
