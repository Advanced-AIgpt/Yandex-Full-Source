GO_LIBRARY()

OWNER(
    g:amelie
    alkapov
)

PEERDIR(
    vendor/go.mongodb.org/mongo-driver/mongo
    vendor/go.mongodb.org/mongo-driver/mongo/options
    vendor/go.mongodb.org/mongo-driver/x/bsonx
)

SRCS(
    mongo.go
)

END()
