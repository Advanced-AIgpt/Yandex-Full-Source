PROGRAM(wonderlogs_creator)

ALLOCATOR(SYSTEM)

OWNER(g:wonderlogs)

SRCS(
    main.cpp
)

PEERDIR(
    ads/bsyeti/big_rt/lib/utility/profiling
    ads/bsyeti/big_rt/lib/processing/resharder/lib
    ads/bsyeti/libs/profiling/solomon
    ads/bsyeti/libs/ytex/http
    ads/bsyeti/libs/ytex/program

    alice/wonderlogs/rt/library/common

    alice/wonderlogs/rt/processors/wonderlogs_creator/lib

    quality/user_sessions/rt/lib/state_managers/proto
)

END()
