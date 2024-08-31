LIBRARY()

OWNER(
    stupnik
    g:hollywood
    g:alice
)

JOIN_SRCS(
    all.cpp
    xml_resp_parser.cpp
    download_info.cpp
    track_quality_selector.cpp
    download_info_parser.cpp
    signature_token.cpp
    track_url_builder.cpp
)

GENERATE_ENUM_SERIALIZATION(download_info.h)

PEERDIR(
    alice/library/json
    alice/library/logger
    contrib/libs/expat
    contrib/libs/openssl
    library/cpp/digest/md5
    library/cpp/string_utils/base64
    library/cpp/string_utils/quote
)

END()
