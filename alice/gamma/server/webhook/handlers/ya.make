GO_LIBRARY()

OWNER(
    g-kostin
    g:alice
)

PEERDIR(
    alice/gamma/server/log
    alice/gamma/server/skills
    alice/gamma/server/storage
    alice/gamma/server/webhook/api
    vendor/golang.org/x/xerrors
    vendor/google.golang.org/grpc
)

SRCS(
    admin.go
    skill.go
)

GO_TEST_SRCS(admin_test.go)

END()

RECURSE(gotest)

RECURSE_FOR_TESTS(gotest)
