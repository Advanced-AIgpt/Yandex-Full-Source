PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:megamind)

PEERDIR(
    alice/megamind/library/scenarios/features/protos
    alice/megamind/protos/common
    alice/megamind/protos/speechkit
)

SRCS(
    response.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
