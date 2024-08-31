GO_PROGRAM(bebeder)

OWNER(
    alkapov
)

PEERDIR(
    alice/beggins/pkg/begemot
    yt/go/schema
    yt/go/ypath
    yt/go/yt
    yt/go/yt/ythttp

    vendor/github.com/go-resty/resty/v2
    vendor/go.uber.org/zap
)

SRCS(
    main.go
)

END()
