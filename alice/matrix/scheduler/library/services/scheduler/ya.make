LIBRARY()

OWNER(
    g:matrix
)

SRCS(
    add_scheduled_action_request.cpp
    remove_scheduled_action_request.cpp
    schedule_http_request.cpp
    service.cpp
    unschedule_http_request.cpp
    utils.cpp
)

PEERDIR(
    alice/matrix/scheduler/library/services/common_context
    alice/matrix/scheduler/library/services/scheduler/protos
    alice/matrix/scheduler/library/storages/scheduler

    alice/matrix/library/request
    alice/matrix/library/services/iface
    alice/matrix/library/services/typed_apphost_service

    alice/megamind/api/utils
)

END()

RECURSE(
    protos
)

RECURSE_FOR_TESTS(
    ut
)
