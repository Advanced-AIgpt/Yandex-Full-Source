LIBRARY()

OWNER(g:bass)

SRCS(
    context.cpp
    runner.cpp
    source.cpp
)

PEERDIR(
    alice/bass/libs/fetcher
    alice/bass/libs/logging_v2
    alice/bass/libs/source_request
    library/cpp/scheme
    search/app_host_ops
    apphost/lib/converter
)

END()
