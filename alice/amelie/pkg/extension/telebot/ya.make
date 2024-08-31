GO_LIBRARY()

OWNER(
    g:amelie
    alkapov
)

PEERDIR(
    alice/amelie/pkg/logging
    alice/amelie/pkg/telegram
    library/go/core/log/ctxlog

    vendor/github.com/go-resty/resty/v2
    vendor/gopkg.in/tucnak/telebot.v2
)

SRCS(
    bot.go
    util.go
)

END()
