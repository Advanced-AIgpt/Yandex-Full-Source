LIBRARY()

OWNER(
    akhruslan
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/config
    alice/hollywood/library/dispatcher
    alice/hollywood/library/global_context
    alice/hollywood/library/http_proxy
    alice/hollywood/library/metrics
    alice/library/logger
    alice/library/metrics
    library/cpp/getopt
    library/cpp/getoptpb
    library/cpp/monlib/encode
    library/cpp/monlib/metrics
    library/cpp/unistat
)

SRCS(
    runner.cpp
)

END()
