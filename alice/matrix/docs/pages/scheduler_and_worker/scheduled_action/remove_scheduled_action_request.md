# Как удалить scheduled action

Для этого надо воспользоваться следующим [типизированным сервисом](https://docs.yandex-team.ru/apphost/pages/typed_api_cpp) scheduler'а:

{% code "/alice/matrix/scheduler/library/services/scheduler/protos/service.proto" lang="protobuf" lines="[BEGIN TRemoveScheduledActionHandler]-[END TRemoveScheduledActionHandler]" %}

{% code "/alice/matrix/scheduler/library/services/scheduler/protos/service.proto" lang="protobuf" lines="[BEGIN TRemoveScheduledActionRequest]-[END TRemoveScheduledActionRequest]" %}

{% code "/alice/matrix/scheduler/library/services/scheduler/protos/service.proto" lang="protobuf" lines="[BEGIN TRemoveScheduledActionResponse]-[END TRemoveScheduledActionResponse]" %}

Найти protobuf'ы request'ов/response'ов [можно тут](https://a.yandex-team.ru/arc/trunk/arcadia/alice/protos/api/matrix/scheduler_api.proto).

## NApi.TRemoveScheduledActionRequest {#api_request}

{% code "/alice/protos/api/matrix/scheduler_api.proto" lang="protobuf" lines="[BEGIN TRemoveScheduledActionRequest]-[END TRemoveScheduledActionRequest]" %}

* ```ActionId``` - ```Id``` из [scheduled action meta](https://docs.yandex-team.ru/alice-matrix/pages/scheduler_and_worker/scheduled_action/#meta)

## NApi.TRemoveScheduledActionResponse {#api_response}

{% code "/alice/protos/api/matrix/scheduler_api.proto" lang="protobuf" lines="[BEGIN TRemoveScheduledActionResponse]-[END TRemoveScheduledActionResponse]" %}

В случае успеха будет возвращен данный protobuf, в случае ошибки будет возвращен apphost'овый exception.
