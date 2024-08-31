GO_LIBRARY()

OWNER(
    g:amanda
    alkapov
)

PEERDIR(
    alice/amanda/internal/app/models

    vendor/gopkg.in/tucnak/telebot.v2
)

SRCS(
    telebot.go
)

END()
