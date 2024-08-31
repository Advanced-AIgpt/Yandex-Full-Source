LIBRARY()

OWNER(
    osado
    petrk
    g:bass
)

PEERDIR(
    alice/bass/libs/logging_v2
    alice/library/network
    alice/library/util
    library/cpp/cgiparam
    library/cpp/http/io
    library/cpp/http/misc
    library/cpp/http/server
    library/cpp/neh
    library/cpp/scheme
    library/cpp/streams/factory
    library/cpp/streams/lzma
    library/cpp/string_utils/base64
    library/cpp/string_utils/quote
    library/cpp/uri
    library/cpp/deprecated/atomic
)

SRCS(
    event_logger.cpp
    instance_counted.cpp #TODO: move to a separated place
    neh.cpp
    neh_detail.cpp
    request.cpp
    serialization.cpp
    util.cpp
)

GENERATE_ENUM_SERIALIZATION(request.h)

END()

RECURSE_FOR_TESTS(
    ut
)
