PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    deemonasd
    g:alice_boltalka
)

SRCS(
    nlgsearch_factors.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
