LIBRARY()

OWNER(g:hollywood)

SRCS(
    global_context.cpp
)

PEERDIR(
    alice/hollywood/library/config
    alice/hollywood/library/fast_data
    alice/hollywood/library/nlg
    alice/hollywood/library/resources
    alice/hollywood/protos
    alice/library/logger
    alice/library/metrics
    alice/library/metrics/sensors_dumper
    apphost/api/service/cpp
)

END()
