GO_LIBRARY()

OWNER(
    g:amelie
    alkapov
)

PEERDIR(
    vendor/github.com/go-resty/resty/v2
)

SRCS(
    client.go
)

END()
