LIBRARY()

OWNER(g:bass)

PEERDIR(
    alice/rtlog/client
    alice/rtlog/protos
    library/cpp/logger/global
)

SRCS(
    log_types.cpp
    logger.cpp
)

GENERATE_ENUM_SERIALIZATION(logger.h)

END()
