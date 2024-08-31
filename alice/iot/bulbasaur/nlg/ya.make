GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    lib_action.go
    lib_cars.go
    lib_common.go
    lib_delayed.go
    lib_error.go
)

GO_TEST_SRCS(lib_delayed_test.go)

END()

RECURSE(gotest)
