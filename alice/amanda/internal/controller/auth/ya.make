GO_LIBRARY()

OWNER(
    g:amanda
    alkapov
)

PEERDIR(
    alice/amanda/internal/app
    alice/amanda/internal/session
    alice/amanda/internal/staff

    vendor/gopkg.in/tucnak/telebot.v2
)

SRCS(
    controller.go
)

END()
