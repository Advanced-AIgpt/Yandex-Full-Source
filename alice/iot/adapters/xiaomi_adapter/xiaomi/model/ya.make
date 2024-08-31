GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    action.go
    const.go
    discovery.go
    model.go
    query.go
    subscription.go
    user.go
)

GO_TEST_SRCS(model_test.go)

END()

RECURSE(gotest)
