GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    const.go
    controller.go
    icontroller.go
    model.go
    pushes.go
    testing.go
)

GO_TEST_SRCS(
    controller_test.go
    discovery_test.go
    model_test.go
    suite_test.go
)

END()

RECURSE_FOR_TESTS(gotest)
