GO_LIBRARY()

OWNER(
    g:amelie
    alkapov
)

PEERDIR(
    library/go/core/log/ctxlog
    library/go/core/log
    library/go/core/log/nop

    vendor/github.com/go-resty/resty/v2
)

SRCS(
    logging.go
    resty.go
)

GO_TEST_SRCS(
    logging_test.go
)

END()

RECURSE(
    gotest
)
