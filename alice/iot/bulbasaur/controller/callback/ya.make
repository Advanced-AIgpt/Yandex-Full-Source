GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    controller.go
    errors.go
    interface.go
)

GO_TEST_SRCS(
    controller_callback_states_test.go
    controller_test.go
    suite_test.go
)

END()

RECURSE(gotest)
