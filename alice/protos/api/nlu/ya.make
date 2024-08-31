PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    vl-trifonov
    g:alice_quality
)

SRCS(
    feature_container.proto
)

END()

RECURSE(generated)
