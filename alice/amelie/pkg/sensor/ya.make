GO_LIBRARY()

OWNER(
    g:amelie
    alkapov
)

PEERDIR(
    alice/library/go/metrics
    library/go/core/metrics
    library/go/core/metrics/solomon
)

SRCS(
    signal.go
)

END()
