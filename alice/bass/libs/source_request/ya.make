LIBRARY()

OWNER(g:bass)

PEERDIR(
    alice/bass/libs/config
    alice/bass/libs/fetcher
    alice/bass/libs/logging_v2
    alice/bass/libs/metrics
    alice/bass/util

    alice/library/client
    alice/library/network

    library/cpp/scheme
)

SRCS(
    handle.cpp
    source_request.cpp
)

END()
