PROGRAM(static_wonderlogs_creator)

ALLOCATOR(SYSTEM)

OWNER(g:wonderlogs)

SRCS(
    main.cpp
)

PEERDIR(
    alice/wonderlogs/rt/library/common

    alice/wonderlogs/rt/processors/static_wonderlogs_creator/lib
    alice/wonderlogs/rt/processors/static_wonderlogs_creator/protos

    ads/bsyeti/libs/profiling/solomon
    ads/bsyeti/libs/ytex/http
    ads/bsyeti/libs/ytex/program
)

END()
