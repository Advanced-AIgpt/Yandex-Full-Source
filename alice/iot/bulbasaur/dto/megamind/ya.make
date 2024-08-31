GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    apply_arguments.go
    const.go
    hypothesis.go
)

GO_TEST_SRCS(
    apply_arguments_test.go
    hypothesis_test.go
)

END()

RECURSE(gotest)
