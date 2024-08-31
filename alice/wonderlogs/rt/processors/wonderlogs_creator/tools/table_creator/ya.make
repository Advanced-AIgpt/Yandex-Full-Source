PROGRAM()

OWNER(g:wonderlogs)

SRCS(
    main.cpp
)

PEERDIR(
    alice/wonderlogs/rt/processors/wonderlogs_creator/lib

    mapreduce/yt/client

    kernel/yt/dynamic
)

END()
