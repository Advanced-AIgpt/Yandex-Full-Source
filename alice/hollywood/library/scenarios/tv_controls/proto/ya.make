PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    dandex
    g:smarttv
)

SRCS(
    tv_controls.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()