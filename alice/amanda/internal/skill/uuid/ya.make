GO_LIBRARY()

OWNER(
    g:amanda
)

PEERDIR(
    alice/amanda/internal/app
    alice/amanda/internal/skill/common

    vendor/gopkg.in/tucnak/telebot.v2
)

SRCS(
    uuid.go
)

END()
