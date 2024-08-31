GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    controller.go
    icontroller.go
    metrics.go
    testing.go
)

GO_TEST_SRCS(controller_test.go)

END()

RECURSE_FOR_TESTS(gotest)
