LIBRARY()

OWNER(g:bass)

PEERDIR(
    alice/bass/libs/config
    alice/rtlog/client

    library/cpp/scheme
)

SRCS(
    rtlog.cpp
)

END()
