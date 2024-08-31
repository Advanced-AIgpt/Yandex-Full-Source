GO_LIBRARY()

OWNER(
    g:amanda
    alkapov
)

PEERDIR(
    vendor/go.mongodb.org/mongo-driver/bson
    vendor/go.mongodb.org/mongo-driver/bson/primitive
    vendor/go.mongodb.org/mongo-driver/mongo
    vendor/go.mongodb.org/mongo-driver/mongo/options
    vendor/go.mongodb.org/mongo-driver/x/bsonx
    vendor/go.uber.org/zap
)

SRCS(
    inmemory.go
    mongo.go
    session.go
    storage.go
)

END()
