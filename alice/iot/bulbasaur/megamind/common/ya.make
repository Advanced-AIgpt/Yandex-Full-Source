GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    action_intent_parameters.go
    apply_arguments.go
    client_info.go
    const.go
    context.go
    continue_arguments.go
    datetime.go
    filtration.go
    origin.go
    query_intent_parameters.go
    slots.go
    specify_request_state.go
    user_info.go
    validation_errors.go
)

GO_TEST_SRCS(
    action_intent_parameters_test.go
    datetime_test.go
    query_intent_parameters_test.go
    slots_test.go
)

END()

RECURSE(gotest)
