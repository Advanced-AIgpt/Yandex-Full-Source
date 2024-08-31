PROTO_LIBRARY(divkt-renderer-grpc)

OWNER(g:smarttv)

EXCLUDE_TAGS(GO_PROTO PY3_PROTO CPP_PROTO)

PEERDIR(
    apphost/proto/extensions
    apphost/lib/proto_answers
    alice/protos/api/renderer
    alice/megamind/protos/scenarios
)

SRCS(
    divkt_renderer_grpc.proto
)

JAVA_PROTO_PLUGIN(java_plugin apphost/tools/stub_generator/java_plugin DEPS apphost/api/service/java/apphost)

END()
