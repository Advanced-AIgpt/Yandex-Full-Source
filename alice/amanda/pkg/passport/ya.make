GO_LIBRARY()

OWNER(
    g:amanda
    alkapov
)

PEERDIR(
    vendor/github.com/go-resty/resty/v2
)

SRCS(
    passport.go
)

END()