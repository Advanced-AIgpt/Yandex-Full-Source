PROTO_LIBRARY()

OWNER(
    g:matrix
)

EXCLUDE_TAGS(GO_PROTO)

PEERDIR(
    alice/matrix/analytics/protos
)

SRCS(
    test_enum_value_ordering.proto
)

END()