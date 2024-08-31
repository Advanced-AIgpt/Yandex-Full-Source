GO_LIBRARY()

OWNER(
    g:amanda
    alkapov
)

PEERDIR(
    alice/amanda/internal/app
    alice/amanda/internal/avatars
    alice/amanda/internal/divrenderer
    alice/amanda/internal/hash
    alice/amanda/internal/session
    alice/amanda/internal/skill/common
    alice/amanda/internal/skill/params
    alice/amanda/internal/uaas
    alice/amanda/internal/xiva
    alice/amanda/pkg/speechkit
    alice/amanda/pkg/uniproxy

    vendor/gopkg.in/tucnak/telebot.v2
)

SRCS(
    controller.go
    helper.go
)

END()
