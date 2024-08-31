PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:alice_analytics
    g:alice_quality
)

PEERDIR(
    alice/protos/data/language
)

SRCS(
    dataset_info.proto
)

END()

RECURSE_FOR_TESTS(
    validation_test
)
