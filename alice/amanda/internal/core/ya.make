GO_LIBRARY()

OWNER(
    g:amanda
    alkapov
)

PEERDIR(
    alice/amanda/internal/adapter/telebot
    alice/amanda/internal/app
    alice/amanda/internal/app/models
    alice/amanda/internal/avatars
    alice/amanda/internal/controller/auth
    alice/amanda/internal/controller/core
    alice/amanda/internal/controller/errorcapture
    alice/amanda/internal/controller/eventlog
    alice/amanda/internal/controller/uniproxy
    alice/amanda/internal/core/config
    alice/amanda/internal/core/metrics/decorators
    alice/amanda/internal/divrenderer
    alice/amanda/internal/editor
    alice/amanda/internal/linker
    alice/amanda/internal/passport
    alice/amanda/internal/product
    alice/amanda/internal/sensors
    alice/amanda/internal/session
    alice/amanda/internal/skill/account
    alice/amanda/internal/skill/app
    alice/amanda/internal/skill/common
    alice/amanda/internal/skill/debug
    alice/amanda/internal/skill/device
    alice/amanda/internal/skill/experiments
    alice/amanda/internal/skill/help
    alice/amanda/internal/skill/location
    alice/amanda/internal/skill/params
    alice/amanda/internal/skill/queryparams
    alice/amanda/internal/skill/uuid
    alice/amanda/internal/staff
    alice/amanda/internal/tvm
    alice/amanda/internal/uaas
    alice/amanda/internal/xiva

    library/go/core/metrics
    library/go/core/metrics/solomon
    library/go/yandex/solomon/reporters/puller/httppuller

    vendor/github.com/labstack/echo/v4
    vendor/github.com/labstack/echo/v4/middleware
    vendor/go.mongodb.org/mongo-driver/mongo
    vendor/go.mongodb.org/mongo-driver/mongo/options
    vendor/go.uber.org/zap
    vendor/go.uber.org/zap/zapcore
    vendor/gopkg.in/tucnak/telebot.v2
)

SRCS(
    core.go
)

END()

RECURSE(
    config
    metrics
)
