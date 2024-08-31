LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/library/apphost_request
    alice/library/json
    alice/library/logger
    alice/library/network
    alice/megamind/library/apphost_request/protos
    alice/megamind/library/globalctx
    alice/megamind/library/request/event
    alice/megamind/library/requestctx
    alice/megamind/library/response_meta
    alice/megamind/library/sources

    library/cpp/http/io
    apphost/api/service/cpp
)

SRCS(
    item_adapter.cpp
    item_names.cpp
    node.cpp
    node_names.cpp
    response.cpp
    request_builder.cpp
    util.cpp
)

END()

RECURSE_FOR_TESTS(ut)
