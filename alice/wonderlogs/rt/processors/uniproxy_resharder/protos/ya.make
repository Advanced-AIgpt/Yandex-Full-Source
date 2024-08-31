PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:wonderlogs)

SRCS(
    config.proto
)

PEERDIR(
    ads/bsyeti/big_rt/lib/consuming_system/config
    ads/bsyeti/big_rt/lib/processing/shard_processor/stateful/config
    ads/bsyeti/big_rt/lib/processing/shard_processor/stateless/config
    ads/bsyeti/big_rt/lib/supplier/config
    ads/bsyeti/big_rt/lib/utility/throttler/proto

    ads/bsyeti/libs/profiling/solomon/proto
    ads/bsyeti/libs/tvm_manager/proto
    ads/bsyeti/libs/ytex/http/proto
    ads/bsyeti/libs/ytex/logging/proto

    library/cpp/getoptpb/proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
