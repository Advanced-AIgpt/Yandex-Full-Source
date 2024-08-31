PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    the0
    samoylovboris
    g:alice_quality
)

SRCS(
    address.proto
    apply_arguments.proto
    cart.proto
    state.proto
)

EXCLUDE_TAGS(GO_PROTO)

PEERDIR("alice/megamind/protos/common")

END()
