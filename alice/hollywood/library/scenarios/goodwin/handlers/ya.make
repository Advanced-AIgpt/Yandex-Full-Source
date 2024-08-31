LIBRARY()

OWNER(
    the0
    g:megamind
)

PEERDIR(
    alice/library/json
    alice/library/logger
    alice/library/network
    alice/library/proto
    alice/megamind/library/search/protos
    alice/scenarios/lib
    alice/hollywood/library/framework
    alice/hollywood/library/frame_filler/lib
    library/cpp/json
    library/cpp/string_utils/quote
    library/cpp/string_utils/base64
    library/cpp/string_utils/url
    library/cpp/resource
    library/cpp/uri
    apphost/lib/common
    contrib/libs/protobuf
)

SRCS(
    action.proto
    actions.cpp
    goodwin_handlers.cpp
    search_doc.cpp
)

END()

RECURSE_FOR_TESTS(ut)
