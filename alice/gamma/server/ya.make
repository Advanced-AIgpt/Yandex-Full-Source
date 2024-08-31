GO_PROGRAM(gamma-server)

OWNER(
    g-kostin
    g:alice
)

PEERDIR(
    alice/gamma/server/log
    alice/gamma/server/sdk
    alice/gamma/server/skills
    alice/gamma/server/storage
    alice/gamma/server/webhook
    vendor/gopkg.in/yaml.v2
)

SRCS(main.go)

END()

RECURSE(
    log
    sdk
    skills
    storage
    webhook
    ydb
)
