GO_LIBRARY()

OWNER(
    g:amanda
    alkapov
)

PEERDIR(
    library/go/core/metrics
    library/go/core/metrics/solomon
)

SRCS(
    names.go
    utils.go
)

END()
