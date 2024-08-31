GO_LIBRARY()

OWNER(
    g:amanda
    alkapov
)

PEERDIR(
    vendor/go.uber.org/zap
    vendor/gopkg.in/yaml.v2
)

SRCS(
    config.go
)

END()
