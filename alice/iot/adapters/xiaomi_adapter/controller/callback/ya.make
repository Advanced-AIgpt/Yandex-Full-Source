GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    controller.go
    errors.go
    interface.go
    metrics.go
)

GO_XTEST_SRCS(
    controller_test.go
    errors_test.go
)

END()

RECURSE(gotest)
