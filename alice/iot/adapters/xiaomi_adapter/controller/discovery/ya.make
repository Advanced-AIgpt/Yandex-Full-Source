GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    builder.go
    controller.go
    filter.go
    interface.go
)

GO_XTEST_SRCS(controller_test.go)

END()

RECURSE(gotest)
