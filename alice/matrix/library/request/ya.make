LIBRARY()

OWNER(
    g:matrix
)

PEERDIR(
    alice/matrix/library/logging
    alice/matrix/library/metrics

    alice/cuttlefish/library/cuttlefish/common

    library/cpp/neh
    library/cpp/protobuf/json
    library/cpp/threading/future

    apphost/api/service/cpp
)

SRCS(
    http_request.cpp
    request.cpp
    request_event_patcher.cpp
    typed_apphost_request.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
