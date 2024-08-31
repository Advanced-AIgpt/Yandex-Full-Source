PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:matrix
)

PEERDIR(
    alice/megamind/protos/speechkit
    alice/megamind/protos/scenarios
)

SRCS(
    api.proto
)

END()
