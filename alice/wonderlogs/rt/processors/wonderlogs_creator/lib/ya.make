LIBRARY()

OWNER(g:wonderlogs)

SRCS(
    processor.cpp
    state.cpp
)

PEERDIR(
    alice/wonderlogs/rt/processors/megamind_creator/protos
    alice/wonderlogs/rt/processors/uniproxy_creator/protos
    alice/wonderlogs/rt/processors/wonderlogs_creator/protos
    alice/wonderlogs/rt/protos

    ads/bsyeti/big_rt/lib/processing/shard_processor/stateful
    ads/bsyeti/big_rt/lib/consuming_system/config
    ads/bsyeti/big_rt/lib/processing/shard_processor/stateful/config
    ads/bsyeti/big_rt/lib/processing/shard_processor/stateless/config
    ads/bsyeti/big_rt/lib/supplier/config

    quality/user_sessions/rt/lib/common
    quality/user_sessions/rt/lib/yt_client

    alice/wonderlogs/protos
    alice/wonderlogs/library/builders
    alice/wonderlogs/library/common
    alice/wonderlogs/library/parsers
)

END()

# RECURSE_FOR_TESTS(ut)