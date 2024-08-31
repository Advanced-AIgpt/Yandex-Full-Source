GO_LIBRARY()

OWNER(
    g:amanda
    alkapov
)

PEERDIR(
    alice/amanda/internal/app/models
    alice/amanda/internal/core/config
    alice/amanda/internal/sensors
    alice/amanda/internal/session
    alice/amanda/internal/uuid

    library/go/core/metrics
    library/go/core/metrics/solomon

    vendor/go.uber.org/zap
    vendor/gopkg.in/tucnak/telebot.v2
)

SRCS(
    app.go
    context.go
    controller.go
    settings.go
)

END()

RECURSE(
    models
)
