LIBRARY()

OWNER(
    g:alice
    g:alice-alarm-scenario
)

PEERDIR(
    contrib/libs/cctz

    library/cpp/scheme
    library/cpp/timezone_conversion
)

SRCS(
    iso8601.cpp
    parser.cpp
    reader.cpp
    types.cpp
    visitors.cpp
)

GENERATE_ENUM_SERIALIZATION(iso8601.h)
GENERATE_ENUM_SERIALIZATION(parser.h)

END()

RECURSE_FOR_TESTS(
    ut
)
