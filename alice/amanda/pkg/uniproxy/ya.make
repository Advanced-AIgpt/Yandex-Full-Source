GO_LIBRARY()

OWNER(
    g:amanda
    alkapov
)

PEERDIR(
    alice/amanda/internal/uuid
    alice/amanda/pkg/speechkit
    alice/amanda/pkg/uniproxy/internal

    vendor/github.com/gorilla/websocket
    vendor/github.com/mitchellh/mapstructure
)

SRCS(
    client.go
    conn.go
    response.go
    settings.go
)

END()

RECURSE(
    internal
)
