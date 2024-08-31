GO_LIBRARY()

OWNER(
    g:amanda
    alkapov
)

PEERDIR(
    alice/amanda/pkg/speechkit
)

SRCS(
    event.go
    message.go
    speechkit.go
)

END()
