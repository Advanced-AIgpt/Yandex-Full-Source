OWNER(
    g:paskills
)

PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

EXCLUDE_TAGS(GO_PROTO)

PEERDIR(
    alice/paskills/social_sharing/proto/api
    alice/megamind/protos/common
    alice/protos/api/matrix
)

SRCS(
    fetch_document.proto
    list_devices_response.proto
    stateless_document.proto
    send_to_device.proto
)

END()
