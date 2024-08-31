PROTO_LIBRARY(memento_grpc)

OWNER(g:paskills)

EXCLUDE_TAGS(GO_PROTO PY3_PROTO CPP_PROTO)

PEERDIR(
    apphost/proto/extensions
    apphost/lib/proto_answers
    alice/memento/proto
)

SRCS(
    memento_grpc.proto
)

JAVA_PROTO_PLUGIN(java_plugin apphost/tools/stub_generator/java_plugin DEPS apphost/api/service/java/apphost)

END()
