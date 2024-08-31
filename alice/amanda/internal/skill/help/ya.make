GO_LIBRARY()

OWNER(
    g:amanda
    alkapov
)

PEERDIR(
    alice/amanda/internal/app
    alice/amanda/internal/skill/account
    alice/amanda/internal/skill/app
    alice/amanda/internal/skill/common
    alice/amanda/internal/skill/experiments
    alice/amanda/internal/skill/features
    alice/amanda/internal/skill/location
    alice/amanda/internal/skill/params
    alice/amanda/internal/skill/queryparams

    vendor/gopkg.in/tucnak/telebot.v2
)

SRCS(
    example.go
    help.go
)

END()
