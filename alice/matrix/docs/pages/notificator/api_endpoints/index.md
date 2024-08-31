# Куда и как делать запросы

## Инсталляции {#installation}

{% list tabs %}

- Prod

    Prod инсталляция, тут гарантируется uptime и надежность.

    Для походов в http API можно использовать балансер [notificator.alice.yandex.net](http://notificator.alice.yandex.net)

    Для походов в http API в обход балансера можно использовать следующие endpoint_set'ы:
    * [sas/matrix-prestable](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-prestable)
    * [sas/matrix-sas](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-sas)
    * [man/matrix-man](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-man)
    * [vla/matrix-vla](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-vla)

    Для походов в apphosted API можно использовать следующие endpoint_set'ы:
    * [sas/matrix-prestable-grpc](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-prestable)
    * [sas/matrix-sas-grpc](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-sas)
    * [man/matrix-man-grpc](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-man)
    * [vla/matrix-vla-grpc](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-vla)

- Dev

    Dev инсталляция, тут ничего не гарантируется. База может в любой момент полностью очиститься, инсталляция может быть полностью недоступна.

    Для походов в http API можно использовать балансер [notificator-dev.alice.yandex.net](http://notificator-dev.alice.yandex.net)

    Для походов в http API в обход балансера можно использовать следующие endpoint_set'ы:
    * [sas/matrix-dev](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-dev)
    * [man/matrix-dev](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-dev)
    * [vla/matrix-dev](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-dev)

    Для походов в apphosted API можно использовать следующие endpoint_set'ы:
    * [sas/matrix-dev-grpc](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-dev)
    * [man/matrix-dev-grpc](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-dev)
    * [vla/matrix-dev-grpc](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-dev)

- Mock (aka test)

    Mock инсталляция, работает полностью так же как и prod (позволяет регистрироваться device'ам, сохраняет пуши в базу, дает по ним статус, etc), но не посылает пуши в девайсы и не ходит в персональные карточки, просто считает что они вернули ```200 OK``` если поход в них необходим в рамках запроса, ходит в СУП в [dry run режиме](https://doc.yandex-team.ru/sup-api/concepts/push-create/index.html#dry-run-param).

    Это инсталляция нужна для прокачек чтобы случайно никому не послать уведомление. К сожалению, ее назвали ```test```, а не ```mock```, и теперь это сложно изменить.

    Для походов в http API можно использовать балансер [notificator-test.alice.yandex.net](http://notificator-test.alice.yandex.net)

    Для походов в http API в обход балансера можно использовать следующие endpoint_set'ы:
    * [sas/matrix-test](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-test)
    * [man/matrix-test](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-test)
    * [vla/matrix-test](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-test)

    Для походов в apphosted API можно использовать следующие endpoint_set'ы:
    * [sas/matrix-test-grpc](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-test)
    * [man/matrix-test-grpc](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-test)
    * [vla/matrix-test-grpc](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-test)


{% endlist %}

## srcrwr {#srcrwr}

Если вы хотите использовать не prod инсталляцию, вам нужен ```srcrwr``` для вашего девайса, он делается через ```uniProxyUrl```, пример для использования dev инсталляции:

```json
{
...
    "uniProxyUrl": "wss://uniproxy.alice.yandex.net/uni.ws?srcrwr=NOTIFICATOR:http://notificator-dev.alice.yandex.net",
...
}
```

Этот ```srcrwr``` нужен для двух вещей:
* Регистрации вашего девайса в базе подключении (у разных инсталляций разные базы подключений, так что, например, для отправки чего-то через dev вам нужно подключить к этой инсталляции девайс, а не просто сделать запрос в нее)
* Для выполнения серверных директив (директив для uniproxy) подразумевающих запрос в notificator

## Как сделать прямой запрос в notificator {#main-api}

Если вы используете старые http ручки:
* Если это prod, вам нужно запросить доступ до подов notificator'а от вашего сервиса, и делать запросы напрямую в поды
* Если это для тестирования, вам нужно попросить доступ до нужного балансера, и делать запросы через него

{% note info %}

Мы стремимся к тому, чтобы полностью убрать балансер.

К сожалению, это не так просто сделать, ибо надо:
* Обойти всех клиентов
* [Выселить тестовые контура из prod макроса](https://st.yandex-team.ru/ZION-228) (сейчас права разграничиваются доступом до балансера)

Так что сейчас все новые походы в prod контур надо делать **только напрямую в поды**, а в тестовый контур только через балансер.

{% endnote %}

Если вы используете apphosted ручки, надо запросить доступ до подов notificator'а от вашего apphost'а и делать запросы напрямую в поды при помощи apphost'а.

## Как сделать что-то из кода сценария {#scenario-api}

Чтобы сделать запрос из сценария, можно составить правильную [серверную директиву](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/scenarios/directives.proto?rev=r9453353#L2445), и uniproxy (а, если точнее, VOICE apphost) сделает запрос за вас.

Подробнее эти серверные директивы описаны рядом с описанием запросов, которые они заменяют.
