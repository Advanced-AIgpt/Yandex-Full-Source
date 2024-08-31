GO_LIBRARY()

OWNER(
    g:amelie
    alkapov
)

PEERDIR(
    alice/amelie/pkg/telegram
    alice/library/go/setrace
    library/go/core/log
    library/go/core/log/ctxlog
)

SRCS(
    command.go
)

END()
