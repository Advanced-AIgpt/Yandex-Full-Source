GO_LIBRARY()

OWNER(
    g:amanda
    alkapov
)

PEERDIR(
    alice/amanda/internal/app
    alice/amanda/pkg/staff
)

SRCS(
    service.go
)

END()
