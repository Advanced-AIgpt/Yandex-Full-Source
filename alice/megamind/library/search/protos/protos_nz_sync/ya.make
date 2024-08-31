PROTO_LIBRARY()

PROTO_NAMESPACE(GLOBAL alice/megamind/library/search/protos/protos_nz_sync)
PY_NAMESPACE(.)

SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:megamind
)

SRCS(
    proxied_alice_meta_info.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
