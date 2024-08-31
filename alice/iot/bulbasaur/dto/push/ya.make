GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    const.go
    request_discovery.go
    response.go
)

END()

RECURSE(gotest)
