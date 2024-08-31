LIBRARY()

OWNER(
    g:alice
    petrk
)

SRCS(
    direct_gallery.cpp
    prepare_search_request.cpp
    strip_alice_meta_info.cpp
    websearch.cpp
)

PEERDIR(
    alice/library/analytics/common
    alice/library/client
    alice/library/experiments
    alice/library/network
    alice/library/geo
    alice/library/restriction_level
    alice/library/util
    alice/megamind/library/search/protos
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    kernel/geodb
    library/cpp/cgiparam
    library/cpp/iterator
    library/cpp/geobase
    library/cpp/openssl/crypto
    library/cpp/string_utils/base64
    library/cpp/string_utils/scan
    yweb/webdaemons/icookiedaemon/icookie_lib/utils
)

GENERATE_ENUM_SERIALIZATION(websearch.h)

END()

RECURSE(
    response
)

RECURSE_FOR_TESTS(ut)
