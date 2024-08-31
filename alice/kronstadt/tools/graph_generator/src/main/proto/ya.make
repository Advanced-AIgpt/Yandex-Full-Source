PROTO_LIBRARY()

# apphost graph jsons have "monitoring" and "settings" at root level but no such Proto is defined

EXCLUDE_TAGS(GO_PROTO CPP_PROTO PY_PROTO PY3_PROTO)

PEERDIR(
    apphost/lib/proto_config/types_nora
)

SRCS("graph_wrapper.proto")

END()
