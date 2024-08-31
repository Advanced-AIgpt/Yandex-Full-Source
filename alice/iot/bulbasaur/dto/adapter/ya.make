GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    action.go
    const.go
    delete.go
    discovery.go
    jsonrpc.go
    quasar.go
    query.go
    remotes.go
    rename.go
    sorting.go
    unlink.go
)

GO_TEST_SRCS(
    action_test.go
    discovery_test.go
    quasar_test.go
    query_test.go
)

END()

RECURSE(gotest)
