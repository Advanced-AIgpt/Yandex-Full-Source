GO_LIBRARY()

OWNER(
    g-kostin
    g:alice
)

PEERDIR(
    alice/gamma/server/log
    alice/gamma/server/ydb
    kikimr/public/sdk/go/ydb
    vendor/golang.org/x/xerrors
)

SRCS(
    inmemory_storage.go
    proxy_storage.go
    storage.go
    ydb_storage.go
)

END()
