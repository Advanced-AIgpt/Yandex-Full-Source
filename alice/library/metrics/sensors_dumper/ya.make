LIBRARY()

OWNER(g:hollywood)

SRCS(
    sensors_dumper.cpp
)

PEERDIR(
    alice/library/metrics
    library/cpp/monlib/encode
    library/cpp/monlib/encode/json
    library/cpp/monlib/metrics
    library/cpp/unistat
)

END()
