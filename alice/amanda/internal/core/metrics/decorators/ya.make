GO_LIBRARY()

OWNER(
    g:amanda
    alkapov
)

PEERDIR(
    alice/amanda/internal/app
    alice/amanda/internal/avatars
    alice/amanda/internal/divrenderer
    alice/amanda/internal/passport
    alice/amanda/internal/sensors
    alice/amanda/internal/staff
    alice/amanda/internal/uaas
    alice/amanda/internal/xiva
    alice/amanda/pkg/uniproxy

    library/go/core/metrics
)

SRCS(
    avatars.go
    divrenderer.go
    passport.go
    staff.go
    uaas.go
    xiva.go
)

END()
