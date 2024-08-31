PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:smarttv
)

PEERDIR(
    alice/protos/data/tv
    alice/protos/data/search_result
)

SRCS(
    video_scene_args.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
