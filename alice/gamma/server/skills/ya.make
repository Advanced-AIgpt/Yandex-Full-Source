GO_LIBRARY()

OWNER(
    g-kostin
    g:alice
)

PEERDIR(vendor/golang.org/x/xerrors)

SRCS(
    inmemory_provider.go
    provider.go
    skill.go
)

END()
