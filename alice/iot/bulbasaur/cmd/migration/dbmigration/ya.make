GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    chunkers.go
    client.go
    client_helpers.go
    helpers.go
    migration.go
)

GO_TEST_SRCS(
    chunkers_test.go
    client_helpers_test.go
    client_test.go
)

END()

RECURSE(gotest)
