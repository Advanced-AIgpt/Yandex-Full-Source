LIBRARY()

OWNER(g:bass)

PEERDIR(
    library/cpp/monlib/metrics
    library/cpp/monlib/service/pages
    library/cpp/unistat
    library/cpp/unistat/idl
)

SRCS(
    metrics.cpp
    place.cpp
    signals.cpp
)

END()

RECURSE_FOR_TESTS(ut)
