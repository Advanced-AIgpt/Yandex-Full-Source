PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:wonderlogs)

SRCS(
    asr_prepared.proto
    banned_user.proto
    error_threshold_config.proto
    megamind_prepared.proto
    private_user.proto
    request_stat.proto
    uniproxy_prepared.proto
    wonderlogs.proto
    wonderlogs_diff.proto
)

PEERDIR(
    alice/library/censor/protos
    alice/library/client/protos
    alice/library/field_differ/protos
    alice/megamind/protos/common
    alice/megamind/protos/speechkit

    mapreduce/yt/interface/protos
    voicetech/library/proto_api
)

EXCLUDE_TAGS(GO_PROTO)

END()
