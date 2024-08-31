GO_LIBRARY()

OWNER(
    g:amelie
    alkapov
)

PEERDIR(
    alice/amelie/internal/model
    alice/amelie/pkg/logging
    alice/amelie/pkg/mongo
    library/go/core/log/ctxlog

    vendor/github.com/gofrs/uuid
    vendor/go.mongodb.org/mongo-driver/bson
    vendor/go.mongodb.org/mongo-driver/mongo
    vendor/go.mongodb.org/mongo-driver/mongo/options
)

SRCS(
    client.go
    db.go
    disk.go
    inmemory.go
)

END()
