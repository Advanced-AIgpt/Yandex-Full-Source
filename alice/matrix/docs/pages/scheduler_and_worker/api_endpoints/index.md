# Куда и как делать запросы

Публичным api к scheduler'у и worker'у является api scheduler'а. На каждый запрос пользователя scheduler делает элементарные запросы в базу, кладя действия в _очередь на исполнение_.
Worker же не имеет публичного api и является offline processor'ом, выполняющем действия из _очереди на исполнение_.

Так что все запросы надо делать только в scheduler.

## Инсталляции {#installation}

{% list tabs %}

- Prod

    Prod инсталляция, тут гарантируется uptime и надежность.

    Для походов в apphosted API scheduler'а можно использовать следующие endpoint_set'ы:
    * [sas/matrix-scheduler-prestable-grpc](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-scheduler-prestable)
    * [sas/matrix-scheduler-sas-grpc](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-scheduler-sas)
    * [man/matrix-scheduler-man-grpc](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-scheduler-man)
    * [vla/matrix-scheduler-vla-grpc](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-scheduler-vla)

- Test

    Dev инсталляция, тут ничего не гарантируется. База может в любой момент полностью очиститься, инсталляция может быть полностью недоступна.

    Работает с [dev инсталляцией notificator'а](https://docs.yandex-team.ru/alice-matrix/pages/notificator/api_endpoints/#installation).

    Для походов в apphosted API scheduler'а можно использовать следующие endpoint_set'ы:
    * [sas/matrix-scheduler-test-grpc](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-scheduler-test)
    * [man/matrix-scheduler-test-grpc](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-scheduler-test)
    * [vla/matrix-scheduler-test-grpc](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-scheduler-test)


{% endlist %}

## srcrwr {#srcrwr}

Если вы хотите использовать не prod инсталляцию, вам нужен ```srcrwr``` для вашего девайса, он делается через ```uniProxyUrl```, пример для использования test инсталляции:

```json
{
...
    "uniProxyUrl": "wss://uniproxy.alice.yandex.net/uni.ws?srcrwr=VOICE__MATRIX_SCHEDULER:VOICE__MATRIX_SCHEDULER_TESTING",
...
}
```

Также вам будет необходимо [добавить ```srcrwr```](https://docs.yandex-team.ru/alice-matrix/pages/notificator/api_endpoints/#srcrwr) для работы с нужной инсталляцией notificator'а.

## Как сделать прямой запрос в notificator {#main-api}

Надо запросить доступ до подов scheduler'а от вашего apphost'а и делать запросы напрямую в поды при помощи apphost'а.

## Как сделать что-то из кода сценария {#scenario-api}

Чтобы сделать запрос из сценария, можно составить правильную [серверную директиву](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/scenarios/directives.proto?rev=r9453353#L2445), и uniproxy (а, если точнее, VOICE apphost) сделает запрос за вас.

Подробнее эти серверные директивы описаны рядом с описанием запросов, которые они заменяют.
