GO_LIBRARY()

OWNER(
    g-kostin
    g:alice
)

PEERDIR(
    alice/gamma/sdk/api
    alice/gamma/server/log
    alice/gamma/server/skills
    alice/gamma/server/storage
    alice/gamma/server/webhook/handlers
    alice/gamma/server/webhook/api
    kikimr/public/sdk/go/ydb
    vendor/golang.org/x/xerrors
)

SRCS(views.go)

END()

RECURSE(
    api
    handlers
)
