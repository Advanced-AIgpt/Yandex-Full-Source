GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    handlers.go
    server.go
)

END()

RECURSE(gotest)
