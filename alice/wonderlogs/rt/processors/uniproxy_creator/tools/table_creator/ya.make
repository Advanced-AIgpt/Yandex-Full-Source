PROGRAM()

OWNER(g:wonderlogs)

SRCS(
    main.cpp
)

PEERDIR(
    alice/wonderlogs/rt/processors/uniproxy_creator/lib

    mapreduce/yt/client

    kernel/yt/dynamic
)

END()