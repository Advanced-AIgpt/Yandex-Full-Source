PROGRAM(uniproxy_resharder)

ALLOCATOR(SYSTEM)

OWNER(g:wonderlogs)

SRCS(
    main.cpp
)

PEERDIR(
    alice/wonderlogs/rt/library/common
    alice/wonderlogs/rt/processors/uniproxy_resharder/lib

    ads/bsyeti/libs/profiling/solomon
    ads/bsyeti/libs/ytex/http
    ads/bsyeti/libs/ytex/program

    library/cpp/getoptpb/proto
)

END()
