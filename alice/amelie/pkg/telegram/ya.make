GO_LIBRARY()

OWNER(
    g:amelie
    alkapov
)

PEERDIR(
    alice/amelie/pkg/logging
    alice/amelie/pkg/util
    alice/library/go/setrace
    library/go/core/log
    library/go/core/log/ctxlog

    vendor/github.com/go-resty/resty/v2
    vendor/gopkg.in/tucnak/telebot.v2
)

SRCS(
    app.go
    event.go
    helper.go
    interface.go
    model.go
    util.go
)

END()

RECURSE(
    interceptor
)
