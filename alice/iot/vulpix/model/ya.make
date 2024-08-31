GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    const.go
    device_state.go
    errors.go
    found_device_info.go
)

GO_TEST_SRCS(device_state_test.go)

END()

RECURSE_FOR_TESTS(gotest)
