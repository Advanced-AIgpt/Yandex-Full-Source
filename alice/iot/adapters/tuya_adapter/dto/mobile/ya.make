GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    const.go
    model.go
    sorting.go
)

GO_TEST_SRCS(
    const_test.go
    model_test.go
    sorting_test.go
)

END()

RECURSE_FOR_TESTS(gotest)
