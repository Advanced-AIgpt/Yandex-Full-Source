GO_LIBRARY()

OWNER(
    g:amanda
    alkapov
)

PEERDIR(
    alice/amanda/internal/uuid
)

SRCS(
    dialog.go
    div2html.go
)

GO_TEST_SRCS(
    render_test.go
)

END()

RECURSE(
    mds
    rotor
)

RECURSE_FOR_TESTS(
    gotest
)

