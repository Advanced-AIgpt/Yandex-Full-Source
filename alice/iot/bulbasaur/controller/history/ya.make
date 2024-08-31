GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    controller.go
    interface.go
    solomon.go
)

GO_TEST_SRCS(solomon_test.go)

END()

RECURSE(
    db
    gotest
)
