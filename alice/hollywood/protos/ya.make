PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:hollywood)

SRCS(
    bass_request_rtlog_token.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
