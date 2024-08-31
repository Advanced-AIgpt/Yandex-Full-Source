LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/bass/libs/fetcher   # to modify vins request
    alice/library/client
    alice/library/json
    alice/library/logger
    alice/library/network
    alice/megamind/api/request
    alice/megamind/library/config
    alice/megamind/library/globalctx
    alice/megamind/library/request_composite
    alice/megamind/library/request_composite/client
    alice/megamind/library/requestctx
    alice/megamind/protos/common
    alice/megamind/protos/speechkit
    library/cpp/http/io
    library/cpp/cgiparam
    library/cpp/string_utils/url
    contrib/libs/protobuf
)

SRCS(
    request.cpp
    request_build.cpp
    request_parser.cpp
    request_parts.cpp
)

END()

RECURSE_FOR_TESTS(ut)
