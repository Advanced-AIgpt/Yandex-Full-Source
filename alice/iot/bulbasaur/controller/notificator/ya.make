GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    const.go
    controller.go
    error.go
    icontroller.go
    mock.go
    model.go
)

GO_XTEST_SRCS(controller_test.go)

END()

RECURSE_FOR_TESTS(gotest)
