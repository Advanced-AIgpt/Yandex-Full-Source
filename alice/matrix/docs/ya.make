DOCS()

OWNER(
    g:matrix
)

USE_PLANTUML()

DOCS_INCLUDE_SOURCES(
    alice/matrix/notificator/configs/subscriptions.json
    alice/matrix/notificator/tools/ydb_scripts/matrix_notificator_init.ydb

    alice/matrix/scheduler/library/services/scheduler/protos/service.proto
    alice/matrix/scheduler/tools/ydb_scripts/matrix_scheduler_init.ydb

    alice/matrix/worker/tools/ydb_scripts/matrix_worker_init.ydb

    alice/protos/api/matrix/action.proto
    alice/protos/api/matrix/scheduled_action.proto
    alice/protos/api/matrix/scheduler_api.proto
)

END()
