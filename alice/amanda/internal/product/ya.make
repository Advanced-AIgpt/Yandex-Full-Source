GO_LIBRARY()

OWNER(
    g:amanda
    alkapov
)

PEERDIR(
    alice/amanda/internal/sensors
    alice/amanda/internal/session

    library/go/core/metrics

    vendor/go.uber.org/zap
)

SRCS(
    metrics.go
)

END()
