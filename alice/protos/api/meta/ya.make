PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:alice
    g:voicetech-infra
)

SRCS(
    backend.proto
)

END()
