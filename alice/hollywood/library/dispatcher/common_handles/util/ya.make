LIBRARY()

OWNER(g:hollywood)

PEERDIR(
    alice/hollywood/library/framework/proto
    alice/hollywood/library/global_context
    alice/hollywood/library/metrics
    alice/library/metrics

    library/cpp/uri

    apphost/api/service/cpp
)

SRCS(
    util.cpp
)

GENERATE_ENUM_SERIALIZATION(util.h)

END()
