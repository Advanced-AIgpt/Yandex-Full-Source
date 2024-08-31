LIBRARY()

OWNER(g:bass)

SRCS(
    cache.cpp
)

PEERDIR(
    alice/bass/libs/logging_v2
    library/cpp/neh
    library/cpp/resource
    library/cpp/scheme
    library/cpp/threading/future
    library/cpp/xml/document
    library/cpp/deprecated/atomic
)

END()

RECURSE(
    ut
)
