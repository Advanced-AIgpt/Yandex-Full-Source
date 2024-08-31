PROTO_LIBRARY()

OWNER(
    g:voicetech-infra
)

SRCS(
    graph.proto
    gproxy.proto
)

#
#   We don't want to have services or clients written in Go or Python
#
EXCLUDE_TAGS(GO_PROTO PY_PROTO)

END()
