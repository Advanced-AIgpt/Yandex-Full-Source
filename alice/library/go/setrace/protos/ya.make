PROTO_LIBRARY()

OWNER(
    galecore
    ivangromov
    g:alice_iot
)

SET(
    PROTOC_TRANSITIVE_HEADERS
    "no"
)

INCLUDE_TAGS(GO_PROTO)

NO_MYPY()

SRCS(log.proto)

PEERDIR(alice/rtlog/protos)

END()
