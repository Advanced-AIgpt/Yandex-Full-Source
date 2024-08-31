LIBRARY()

OWNER(
    g:megamind
)

PEERDIR(
    alice/wonderlogs/protos
    mapreduce/yt/client
    mapreduce/yt/util
)

SRCS(
    hist.cpp
    mapper.cpp
)

END()

RECURSE_FOR_TESTS(ut)
