# Как создать или обновить существующий scheduled action

Для этого надо воспользоваться следующим [типизированным сервисом](https://docs.yandex-team.ru/apphost/pages/typed_api_cpp) scheduler'а:

{% code "/alice/matrix/scheduler/library/services/scheduler/protos/service.proto" lang="protobuf" lines="[BEGIN TAddScheduledActionHandler]-[END TAddScheduledActionHandler]" %}

{% code "/alice/matrix/scheduler/library/services/scheduler/protos/service.proto" lang="protobuf" lines="[BEGIN TAddScheduledActionRequest]-[END TAddScheduledActionRequest]" %}

{% code "/alice/matrix/scheduler/library/services/scheduler/protos/service.proto" lang="protobuf" lines="[BEGIN TAddScheduledActionResponse]-[END TAddScheduledActionResponse]" %}

Найти protobuf'ы request'ов/response'ов [можно тут](https://a.yandex-team.ru/arc/trunk/arcadia/alice/protos/api/matrix/scheduler_api.proto).

## NApi.TAddScheduledActionRequest {#api_request}

{% code "/alice/protos/api/matrix/scheduler_api.proto" lang="protobuf" lines="[BEGIN TAddScheduledActionRequest]-[END TAddScheduledActionRequest]" %}

* ```Meta``` - [scheduled action meta](https://docs.yandex-team.ru/alice-matrix/pages/scheduler_and_worker/scheduled_action/#meta) содержащая только поле ```Id``` (```Guid``` должен быть пустым)
* ```Spec``` - [scheduled action spec](https://docs.yandex-team.ru/alice-matrix/pages/scheduler_and_worker/scheduled_action/#spec)
* ```OverrideMode``` - режим обновления существующего scheduled action'а:
    * ```NONE``` - в случае отсутствия scheduled action'а в базе он будет создан, иначе будет возвращена ошибка
    * ```META_AND_SPEC_ONLY``` - в случае отсутствия scheduled action'а в базе он будет создан, иначе будут обновлены только ```Meta``` и ```Spec```, ```Status``` изменен не будет (в частности ```StartPolicy``` не будет применена к ```Status.ScheduledAt```). Нужно для обновления ```Spec``` периодичных action'ов без изменения времени следующего выполнения и с сохранением статистики.
    * ```ALL``` - в случае отсутствия scheduled action'а в базе он будет создан, иначе ```Meta``` и ```Spec``` обновятся, а ```Status``` сбросится до дефолтного состояния (по факту это эквивалентно remove + create)

## NApi.TAddScheduledActionResponse {#api_response}

{% code "/alice/protos/api/matrix/scheduler_api.proto" lang="protobuf" lines="[BEGIN TAddScheduledActionResponse]-[END TAddScheduledActionResponse]" %}

В случае успеха будет возвращен данный protobuf, в случае ошибки будет возвращен apphost'овый exception.
