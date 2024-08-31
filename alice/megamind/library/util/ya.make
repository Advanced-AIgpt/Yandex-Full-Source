LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/bass/libs/fetcher
    alice/library/json
    alice/megamind/library/config/protos
    alice/megamind/protos/common
    library/cpp/http/io
    library/cpp/http/misc
    library/cpp/json
    library/cpp/string_utils/base64
)

SRCS(
    config.cpp
    fs.cpp
    slot.cpp
    guid.cpp
    http_response.cpp
    request.cpp
    status.cpp
    str.cpp
    ttl.cpp
    wildcards.cpp
)

GENERATE_ENUM_SERIALIZATION(status.h)

END()

RECURSE_FOR_TESTS(ut)
