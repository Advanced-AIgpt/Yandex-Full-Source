LIBRARY()

OWNER(g:alice)

PEERDIR(
    mapreduce/yt/client
)

SRCS(
    util.cpp
)

END()

RECURSE(
    protos
)
