GO_LIBRARY()

OWNER(
    g:amanda
    alkapov
)

PEERDIR(
    alice/amanda/internal/app
    alice/amanda/internal/skill/common

    vendor/gopkg.in/tucnak/telebot.v2
)

SRCS(
    debug.go
)

END()
