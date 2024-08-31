PROTO_LIBRARY(dialogovo-protos)
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:paskills)

SRCS(
    apply_args.proto
    dialogovo_state.proto
)

PEERDIR(
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    metrika/core/libs/appmetrica/protos/messages
)

EXCLUDE_TAGS(GO_PROTO PY3_PROTO PY_PROTO)

END()

