GO_LIBRARY()

OWNER(
    g:amanda
    alkapov
)

PEERDIR(
    alice/amanda/internal/app
    alice/amanda/internal/passport
    alice/amanda/internal/session
    alice/amanda/internal/skill/common

    vendor/gopkg.in/tucnak/telebot.v2
)

SRCS(
    account.go
)

END()
