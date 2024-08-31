LIBRARY()

OWNER(g:wonderlogs)

SRCS(
    processor.cpp
    state.cpp
)

PEERDIR(
    alice/wonderlogs/rt/processors/megamind_creator/protos
    alice/wonderlogs/rt/protos

    ads/bsyeti/big_rt/lib/processing/shard_processor/stateful

    quality/user_sessions/rt/lib/common
    quality/user_sessions/rt/lib/yt_client

    alice/wonderlogs/protos
    alice/wonderlogs/library/common
    alice/wonderlogs/library/parsers
)

END()
