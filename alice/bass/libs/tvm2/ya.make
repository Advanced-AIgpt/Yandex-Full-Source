LIBRARY()

OWNER(g:bass)

PEERDIR(
    alice/bass/libs/config
    alice/bass/libs/fetcher
    alice/bass/libs/globalctx
    alice/bass/libs/logging_v2
    alice/bass/libs/source_request
    alice/bass/libs/tvm2/ticket_cache
)

SRCS(
    tvm2_ticket_cache.cpp
)

END()
