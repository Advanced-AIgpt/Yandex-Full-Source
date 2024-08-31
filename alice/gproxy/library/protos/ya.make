PROTO_LIBRARY()

OWNER(
    g:voicetech-infra
)

GRPC()

SRCS(
    metadata.proto
    gsetup.proto
    echo.proto
    service.proto
)

PEERDIR(
    alice/gproxy/library/protos/annotations
    alice/protos/data/search_result
    alice/protos/data/tv_feature_boarding
    alice/protos/data/tv/home
    alice/protos/data/tv/watch_list
    alice/protos/data/tv/channels
    alice/protos/data/tv
)


CPP_PROTO_PLUGIN(
    protoc-gen-gproxy alice/gproxy/tools/protoc-gen-gproxy .gproxy.pb.h
)

#
#   We don't want to have services or clients written in Go or Python
#
EXCLUDE_TAGS(GO_PROTO PY_PROTO)

END()


RECURSE(
    annotations
)
