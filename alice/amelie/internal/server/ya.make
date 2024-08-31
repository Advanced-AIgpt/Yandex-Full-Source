GO_LIBRARY()

OWNER(
    g:amelie
    alkapov
)

PEERDIR(
    alice/amelie/internal/config
    alice/amelie/internal/controller
    alice/amelie/internal/db
    alice/amelie/internal/interceptor
    alice/amelie/internal/model
    alice/amelie/internal/provider
    alice/amelie/pkg/bass
    alice/amelie/pkg/extension/telebot
    alice/amelie/pkg/iot
    alice/amelie/pkg/logging
    alice/amelie/pkg/passport
    alice/amelie/pkg/sensor
    alice/amelie/pkg/staff
    alice/amelie/pkg/telegram
    alice/library/go/metrics
    alice/library/go/requestid
    alice/library/go/setrace
    alice/library/go/zaplogger
    library/go/core/log
    library/go/core/log/ctxlog
    library/go/core/log/zap/asynczap
    library/go/core/log/zap
    library/go/core/metrics/solomon
    library/go/yandex/solomon/reporters/puller/httppuller

    vendor/github.com/go-resty/resty/v2
    vendor/github.com/gofrs/uuid
    vendor/github.com/labstack/echo/v4
    vendor/github.com/labstack/echo/v4/middleware
    vendor/go.mongodb.org/mongo-driver/mongo
    vendor/go.mongodb.org/mongo-driver/mongo/options
    vendor/go.uber.org/zap
    vendor/gopkg.in/tucnak/telebot.v2
)

SRCS(
    provider.go
    server.go
    setrace.go
)

END()
