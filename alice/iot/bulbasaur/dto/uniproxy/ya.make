GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    const.go
    user.go
)

GO_TEST_SRCS(
    const_test.go
    user_test.go
)

END()

RECURSE(gotest)
