PROTO_LIBRARY(contacts-protos)
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:paskills)

PEERDIR(
    mapreduce/yt/interface/protos
)

SRCS(
    contacts.proto
)

EXCLUDE_TAGS(GO_PROTO PY3_PROTO PY_PROTO CPP_PROTO)

END()