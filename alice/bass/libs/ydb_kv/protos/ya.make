PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:bass)

SRCS(kv.proto)

EXCLUDE_TAGS(GO_PROTO)

END()
