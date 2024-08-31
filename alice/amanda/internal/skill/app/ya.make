GO_LIBRARY()

OWNER(
    alkapov
    g:amanda
)

PEERDIR(
    alice/amanda/internal/app
    alice/amanda/internal/skill/common
    vendor/gopkg.in/tucnak/telebot.v2
)

SRCS(
    app.go
    presets.go
)

END()

RECURSE(
    generator
)
