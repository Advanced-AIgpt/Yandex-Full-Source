GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    const.go
    controller.go
    errors.go
)

GO_TEST_SRCS(controller_test.go)

END()

RECURSE(gotest)
