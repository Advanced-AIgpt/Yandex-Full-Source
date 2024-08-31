PROTO_LIBRARY()

PROTO_NAMESPACE(GLOBAL alice/megamind/library/search/protos/protos)
PY_NAMESPACE(.)

SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:megamind
)

SRCS(
    proxied_alice_meta_info.proto
)

PEERDIR(
    alice/library/client/protos
    alice/megamind/protos/common
)

EXCLUDE_TAGS(GO_PROTO)

END()
