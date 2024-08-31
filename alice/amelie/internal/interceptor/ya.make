GO_LIBRARY()

OWNER(
    g:amelie
    alkapov
)

PEERDIR(
    alice/amelie/internal/config
    alice/amelie/internal/db
    alice/amelie/internal/model
    alice/amelie/pkg/logging
    alice/amelie/pkg/passport
    alice/amelie/pkg/sensor
    alice/amelie/pkg/staff
    alice/amelie/pkg/telegram
    alice/amelie/pkg/telegram/interceptor
    alice/library/go/setrace
    library/go/core/log
    library/go/core/log/ctxlog
    library/go/core/metrics

    vendor/github.com/gofrs/uuid
)

SRCS(
    auth.go
    cancel.go
    command.go
    ratelimiter.go
    sensor.go
    session.go
    state.go
    yandex.go
)

END()
