PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(gusev-p)

INCLUDE_TAGS(GO_PROTO)

SRCS(
    rtlog.ev
)

PEERDIR(library/cpp/eventlog/proto)

END()
