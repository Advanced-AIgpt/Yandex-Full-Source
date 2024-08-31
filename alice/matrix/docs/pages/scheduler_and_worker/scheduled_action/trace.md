# Как отследить все события с scheduled action'ом по логам

## Логи scheduler'а и worker'а {#scheduler_and_worker}

Отследить все события с scheduled action'ом по логам scheduler'а и worker'а можно при помощи логов в yt, они лежат на hahn в [/logs/matrix-scheduler-prod-eventlog](https://yt.yandex-team.ru/hahn/navigation?path=//home/logfeller/logs/matrix-scheduler-prod-eventlog) и [/logs/matrix-worker-prod-eventlog](https://yt.yandex-team.ru/hahn/navigation?path=//home/logfeller/logs/matrix-worker-prod-eventlog).

Для этого надо воспользоваться [вот этой udf](https://sandbox.yandex-team.ru/resource/3276336323/view) для парса framed лога. [Скоро она появится в аркадии.](https://st.yandex-team.ru/ZION-158)

[Пример запроса с этой udf](https://yql.yandex-team.ru/Operations/YsCYRclwvQzimuoO54A_rK1pmAtgiTayZ8GN6wlWwE4=).
В этом примере указаны оба интересных поля:
1. ```action_id```
2. ```action_guid```

В ```action_guid``` можно указать ```NULL```, и тогда поиск будет по всем версиям scheduled action'а, а не по какой-то конкретной версии.

{% code "./_includes/trace.yql" lang="yql" %}

## Логи notificator'а {#notificator}

Для ```SendTechnicalPush``` action'ов логи scheduler'а и worker'а можно дополнительно провязать с логами notificator'а.

Для этого надо найти event ```MatrixWorkerDoSendTechnicalPushStart``` и взять из него поле ```PushId```:

```json
{
    "ActionId" = "chegoryu-test-p";
    "ActionGuid" = "52185c5-c734d03f-5709ad0a-7c749ec9";
    "MatrixNotificatorHost" = "notificator-dev.alice.yandex.net";
    "MatrixNotificatorPort" = 80;
    "Puid" = "235236314";
    "DeviceId" = "0410789683084c250150";
    "PushId" = "86657e94-2eff562-1a0dbbfb-7bab414f$269530e7-a5fc2d2d-23d727ce-d6d4bce3$ea5d5b3e-43484d00-e771ed2f-a57859d6";
}
```

Далее надо воспользоваться [инструкцией по отслеживанию технического пуша](https://docs.yandex-team.ru/alice-matrix/pages/notificator/technical_pushes/trace) по логам.
