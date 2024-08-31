GO_PROGRAM(amelie)

OWNER(
    g:amelie
    alkapov
)

PEERDIR(
    alice/amelie/internal/config
    alice/amelie/internal/server
    library/go/core/log
    library/go/core/log/zap

    vendor/go.uber.org/zap
)

SRCS(
    main.go
)

END()
