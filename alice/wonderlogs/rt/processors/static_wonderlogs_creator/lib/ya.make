LIBRARY()

OWNER(g:wonderlogs)

SRCS(
    processor.cpp
)

PEERDIR(
    alice/wonderlogs/rt/processors/static_wonderlogs_creator/protos

    ads/bsyeti/big_rt/lib/processing/shard_processor/stateful

    dict/dictutil

    mapreduce/yt/interface
    mapreduce/yt/client

    quality/user_sessions/rt/lib/common
    quality/user_sessions/rt/lib/yt_client

    alice/wonderlogs/protos
    alice/wonderlogs/library/common
    alice/wonderlogs/library/parsers
)

END()

# RECURSE_FOR_TESTS(ut)
