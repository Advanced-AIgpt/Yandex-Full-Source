LIBRARY()

OWNER(g:wonderlogs)

SRCS(
    utils.cpp
)

PEERDIR(
    mapreduce/yt/interface
)

END()

RECURSE_FOR_TESTS(ut)
