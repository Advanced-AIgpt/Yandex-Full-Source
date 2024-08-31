LIBRARY()

OWNER(g:alice)

PEERDIR(
    alice/library/client
    alice/library/geo
    alice/library/restriction_level/protos
    contrib/libs/openssl
    library/cpp/cgiparam
    library/cpp/resource
    library/cpp/string_utils/quote
    library/cpp/uri
)

SRCS(
    url_builder.cpp
)

GENERATE_ENUM_SERIALIZATION(url_builder.h)

END()

RECURSE_FOR_TESTS(ut)
