GO_LIBRARY()

OWNER(
    g:amanda
    alkapov
)

PEERDIR(
    alice/amanda/internal/app

    vendor/gopkg.in/tucnak/telebot.v2
)

SRCS(
    controller.go
)

END()
