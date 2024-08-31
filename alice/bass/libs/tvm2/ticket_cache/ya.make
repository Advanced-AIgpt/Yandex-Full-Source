LIBRARY()

OWNER(g:bass)

PEERDIR(
    alice/bass/libs/fetcher
    alice/bass/util
    library/cpp/cgiparam
    library/cpp/scheme
    library/cpp/tvmauth
)

SRCS(
    ticket_cache.cpp
    ticket_cache_holder.cpp
)

GENERATE_ENUM_SERIALIZATION(ticket_cache.h)
GENERATE_ENUM_SERIALIZATION(ticket_cache_holder.h)

END()
