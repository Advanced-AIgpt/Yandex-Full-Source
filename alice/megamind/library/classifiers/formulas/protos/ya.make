PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    olegator
    g:megamind
)

PEERDIR(
    alice/megamind/protos/quality_storage
    alice/protos/data/language
)

SRCS(
    formulas_description.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
