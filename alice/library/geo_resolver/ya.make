LIBRARY()

OWNER(g:alice)

PEERDIR(
    alice/library/client
    alice/library/geo
    alice/library/restriction_level/protos
    alice/library/url_builder
    library/cpp/cgiparam
    library/cpp/scheme
    library/cpp/timezone_conversion
    maps/doc/proto/yandex/maps/proto/common2
    maps/doc/proto/yandex/maps/proto/search
    search/session/compression
)

SRCS(
    geo_cgi_builder.cpp
    geo_response_parser.cpp
    geo_position.cpp
)

END()
