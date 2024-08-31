LIBRARY()

OWNER(
    g:matrix
)

SRCS(
    loop.cpp
    manual_sync_request.cpp
    service.cpp
    sync.cpp
    utils.cpp
)

PEERDIR(
    alice/matrix/worker/library/services/common_context
    alice/matrix/worker/library/services/worker/protos
    alice/matrix/worker/library/storages/worker

    alice/matrix/library/request
    alice/matrix/library/services/iface
    alice/matrix/library/services/typed_apphost_service

    alice/protos/api/matrix

    library/cpp/http/simple
    library/cpp/protobuf/interop
    library/cpp/watchdog
)

END()

RECURSE(
    protos
)

RECURSE_FOR_TESTS(
    ut
)
