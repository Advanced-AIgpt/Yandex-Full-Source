LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/library/logger/proto
    alice/rtlog/client
    alice/rtlog/protos

    library/cpp/logger
)

SRCS(
    log_types.cpp
    logger.cpp
    logger_utils.cpp
    logadapter.cpp
)

GENERATE_ENUM_SERIALIZATION(logadapter.h)

END()

RECURSE_FOR_TESTS(ut)
