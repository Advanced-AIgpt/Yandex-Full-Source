GO_LIBRARY()

OWNER(
    g:amelie
    alkapov
)

PEERDIR(
    alice/amelie/pkg/logging
    alice/amelie/pkg/sensor
    alice/library/go/metrics
    alice/library/go/setrace
    library/go/core/log
    library/go/core/metrics

    vendor/github.com/go-resty/resty/v2
)



SRCS(
    logging.go
    sensor.go
)

END()
