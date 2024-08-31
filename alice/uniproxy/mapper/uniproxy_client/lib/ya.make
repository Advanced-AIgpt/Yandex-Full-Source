LIBRARY()

OWNER(
    g:alice_downloaders
)

ADDINCL(
    contrib/libs/poco/Crypto/include
    contrib/libs/poco/Foundation/include
    contrib/libs/poco/Net/include
    contrib/libs/poco/Net/include
    contrib/libs/poco/NetSSL_OpenSSL/include
    contrib/libs/poco/Util/include
)


SRCS(
    helpers.cpp
    base_client.cpp
    uniproxy_client.cpp
    async_uniproxy_client.cpp
)

PEERDIR(
    contrib/libs/poco/Net
    contrib/libs/poco/NetSSL_OpenSSL
    library/cpp/cgiparam
    library/cpp/json
    library/cpp/string_utils/url
    library/cpp/timezone_conversion
    library/cpp/uri

    alice/uniproxy/mapper/library/flags
    alice/uniproxy/mapper/library/logging
)

GENERATE_ENUM_SERIALIZATION(uniproxy_client.h)

END()
