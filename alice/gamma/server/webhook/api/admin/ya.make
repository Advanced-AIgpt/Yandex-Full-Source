GO_LIBRARY()

OWNER(
    g-kostin
    g:alice
)

PEERDIR(
    alice/gamma/server/skills
    vendor/golang.org/x/xerrors
)

SRCS(
    request.go
    response.go
)

END()
