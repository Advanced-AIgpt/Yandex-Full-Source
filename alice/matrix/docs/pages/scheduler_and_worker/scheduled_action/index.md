# Что такое scheduled action

Если коротко, scheduled action это какое-то действие, к которому добавлена информация о том:
* Когда выполнить действие
* Повторять ли действие периодично
* С какими периодами делать retry в случае неудачи выполнения действия

И многое другое.

## Возможные действия {#actions}

Все возможные действия представленны в виде protobuf'а ```TAction```:

{% cut "Full TAction" %}

{% code "/alice/protos/api/matrix/action.proto" lang="protobuf" lines="[BEGIN TAction]-[END TAction]" %}

{% endcut %}

Сам protobuf является большим ```oneof``` на все возможные действия.

Найти полный protobuf [можно тут](https://a.yandex-team.ru/arc/trunk/arcadia/alice/protos/api/matrix/action.proto).

### MockAction {#mock_action}

{% code "/alice/protos/api/matrix/action.proto" lang="protobuf" lines="[BEGIN TMockAction]-[END TMockAction]" %}

Специальный action для тестирования, не должен никем использоваться кроме разработчиков scheduler'а и worker'а.

### SendTechnicalPush {#send_technical_push}

{% code "/alice/protos/api/matrix/action.proto" lang="protobuf" lines="[BEGIN TSendTechnicalPush]-[END TSendTechnicalPush]" %}

Action, позволяющий послать [технический пуш](https://docs.yandex-team.ru/alice-matrix/pages/notificator/technical_pushes) при помощи notificator'а.

## Спецификация {#specification}

{% code "/alice/protos/api/matrix/scheduled_action.proto" lang="protobuf" lines="[BEGIN TScheduledAction]-[END TScheduledAction]" %}

Полный объект scheduled action'а разделен на три логические части:
* ```Meta``` - meta информация о scheduled action'е (id, internal id, etc)
* ```Spec``` - спецификация, содержит описания политики выполнения action'а и сам action
* ```Status``` - статус в котором записано текущее состояние того, что происходило, происходит, и будет происходить с scheduled action'ом (время следующего выполнения action'а, последняя ошибка, происходит ли попытка выполнить action в данный момент, etc)

Найти полный protobuf [можно тут](https://a.yandex-team.ru/arc/trunk/arcadia/alice/protos/api/matrix/scheduled_action.proto).

### Meta {#meta}

{% code "/alice/protos/api/matrix/scheduled_action.proto" lang="protobuf" lines="[BEGIN TScheduledActionMeta]-[END TScheduledActionMeta]" %}

* ```Id``` - id scheduled action'а, которой указал пользователь, является уникальным ключом, по которому можно обновить или удалить scheduled action
* ```Guid``` - внутренний id, перегенерируется при каждом обновлении scheduled action'а, используется чтобы не выполнить старую спецификацию scheduled action'а после её обновления

### Spec {#spec}

{% cut "Full Spec" %}

{% code "/alice/protos/api/matrix/scheduled_action.proto" lang="protobuf" lines="[BEGIN TScheduledActionSpec]-[END TScheduledActionSpec]" %}

{% endcut %}

* ```StartPolicy``` - политика того, в какой момент начать выполнять ```SendPolicy```
* ```SendPolicy``` - политика того, как выполнять action (выполнять до первой успешной попытки, выполнять периодично, через какие периоды делать retry ошибок, etc). Называется ```SendPolicy``` по историческим причинам, на самом деле должна называться ```PerformPolicy```
* ```Action``` - спецификация action'а, который надо выполнять

#### StartPolicy {#start_policy}

{% code "/alice/protos/api/matrix/scheduled_action.proto" lang="protobuf" lines="[BEGIN TStartPolicy]-[END TStartPolicy]" %}

* ```StartAt``` - в какой момент времени в первый раз выполнить ```SendPolicy``` (при создании scheduled action'a ```Status.ScheduledAt``` = ```StartPolicy.StartAt```)

Текущие валидации:
* ```CurrentTime() - 10 minutes <= StartAt```

#### SendPolicy {#send_policy}

{% cut "Full SendPolicy" %}

{% code "/alice/protos/api/matrix/scheduled_action.proto" lang="protobuf" lines="[BEGIN TSendPolicy]-[END TSendPolicy]" %}

{% endcut %}

* ```Policy``` - одна из политик выполнения
    * ```SendOncePolicy``` - Выполнять до первой успешной попытки (или истечения лимита на retry'и)
    * ```SendPeriodicallyPolicy``` - Выполнять периодично
* ```Deadline``` - deadline после которого scheduled actoin'он будет удален из базы, и попыток его исполнить больше производиться не будет. Если отсутствует, считается что ```Deadline``` = $\infty$

##### RetryPolicy {#retry_policy}

{% code "/alice/protos/api/matrix/scheduled_action.proto" lang="protobuf" lines="[BEGIN TRetryPolicy]-[END TRetryPolicy]" %}

Политика retry'ев, несущая идею что retry происходит через:

```cpp
Min(
    MaxRestartPeriod,
    MinRestartPeriod + RestartPeriodScale * (RestartPeriodBackOff ^ ConsecutiveFailuresCounter)
)
```

Но учет параметров ```MaxRetries``` и ```ConsecutiveFailuresCounter``` зависит от непосредственной политики.

Текущие дефолты (если protobuf не задан):
* ```MaxRetries = 0```
* ```RestartPeriodScale = 0```
* ```RestartPeriodBackOff = 0```
* ```MinRestartPeriod = 1 second```
* ```MaxRestartPeriod = 1 second```

Текущие валидации:
* ```1 second <= MinRestartPeriod <= MaxRestartPeriod```

##### SendOncePolicy {#send_once_policy}

{% code "/alice/protos/api/matrix/scheduled_action.proto" lang="protobuf" lines="[BEGIN TSendOncePolicy]-[END TSendOncePolicy]" %}

* ```RetryPolicy``` - будет произведено не более ```1 + MaxRetries``` попыток выполнить action, при каждом неуспехе ```ConsecutiveFailuresCounter``` считается равным ```Status.ConsecutiveFailuresCounter``` **без учета текущей попытки**

После успешной попытки, или истечения лимита на retry'ев, scheduled action удаляется.

##### SendPeriodicallyPolicy {#send_periodically_policy}

{% code "/alice/protos/api/matrix/scheduled_action.proto" lang="protobuf" lines="[BEGIN TSendPeriodicallyPolicy]-[END TSendPeriodicallyPolicy]" %}

* ```Period``` - через какие period'ы выполнять action
* ```RetryPolicy``` - в данный момент не реализована, будет сделана в [рамках этого тикета](https://st.yandex-team.ru/ZION-226) (пока не выбрано как будут учитываться ```MaxRetries``` и ```ConsecutiveFailuresCounter```, есть только черновик идеи)

Текущие валидации:
* ```1 second <= Period```

{% note info %}

В данным момент есть только два способа остановить периодичный action:
* Выставить ```Deadline```
* Удалить явным запросом

В будущем будут добавлены дополнительные механизмы.

{% endnote %}

### Status {#status}

{% code "/alice/protos/api/matrix/scheduled_action.proto" lang="protobuf" lines="[BEGIN TScheduledActionStatus]-[END TScheduledActionStatus]" %}

* ```ScheduledAt``` - время следующего выполнения action'а
* ```SuccessfulAttemptsCounter``` - сколько раз получилось удачно выполнить action
* ```FailedAttemptsCounter``` - сколько раз при **непосредственном выполнении** (например при запросе в notificator) action'а произошла ошибка
* ```InterruptedAttemptsCounter``` - сколько раз попытка выполнения action'а **как процесса** (например при получении action'а из очереди, проверки инвариантов базы, etc) была прервана из-за какой-то внутренней ошибки worker'а (например он упал с segmentation fault, его выключили без graceful shutdown, упала операция в ydb)
* ```ConsecutiveFailuresCounter``` - сколько раз подряд при выполнение action'а происходила **любая** ошибка, сбрасывается в 0, если action выполнился успешно
* ```CurrentAttemptStatus``` - статус текущей попытки выполнить action
* ```LastAttemptStatus``` - статус последней попытки выполнить action

{% note info %}

В случае внутренней ошибки worker'а статус может обновиться до актуального только в момент следующей попытки выполнить action, до этого он будет лежать в базе с незавершенным ```CurrentAttempt```'ом.

{% endnote %}
