# Ссылки для разработчиков/дежурных

## Мониторинг {#monitoring}

### Self monitoring {#self_monitoring}

* [Проект в solomon](https://solomon.yandex-team.ru/admin/projects/matrix)
* [Проект в monitoring](https://monitoring.yandex-team.ru/projects/matrix)
* [Дашборды по ручкам и общий дашборд](https://monitoring.yandex-team.ru/projects/matrix/dashboards)
* [Проверки в juggler](https://juggler.yandex-team.ru/project/matrix/dashboard?project=matrix)

### Uniproxy to matrix {#monitoring_uniproxy_to_matrix}

* [Дашборд uniproxy -> matrix](https://yasm.yandex-team.ru/template/panel/uniproxy_template/environment=prod;locations=sas-pre,sas,man,vla;components=matrix,notificator)
* [Connect'ы и disconnect'ы к uniproxy](https://yasm.yandex-team.ru/chart/signals=unistat-uniprx_ws_open_summ,unistat-uniprx_ws_close_summ;hosts=ASEARCH;itype=uniproxy;ctype=prod/?range=86400000)

## Сервисы в няне {#services}

### Prod {#services_prod}

{% list tabs %}

- Новые cpp сервисы

    * [matrix-prestable](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-prestable)
    * [matrix-sas](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-sas)
    * [matrix-man](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-man)
    * [matrix-vla](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-vla)

- Старые python сервисы

    * [notificator-sas](https://nanny.yandex-team.ru/ui/#/services/catalog/notificator-sas)
    * [notificator-man](https://nanny.yandex-team.ru/ui/#/services/catalog/notificator-man)
    * [notificator-vla](https://nanny.yandex-team.ru/ui/#/services/catalog/notificator-vla)

{% endlist %}

### Testing/dev/etc {#services_unstable}

{% list tabs %}

- Новые cpp сервисы

    * [matrix-test](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-test)
    * [matrix-dev](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-dev)

- Старые python сервисы

    * [notificator-test](https://nanny.yandex-team.ru/ui/#/services/catalog/notificator-test)
    * [notificator-dev](https://nanny.yandex-team.ru/ui/#/services/catalog/notificator-dev)

{% endlist %}

### Дашборды {#services_dashboards}

* [Новые cpp сервисы](https://nanny.yandex-team.ru/ui/#/services/dashboards/catalog/matrix)
* [Старые python сервисы](https://nanny.yandex-team.ru/ui/#/services/dashboards/catalog/py_notificator)

## Базы в YDB {#ydb}

{% list tabs %}

- New UI

    * [Все базы вместе](https://yc.yandex-team.ru/folders/foodpffaf31u2gv7s37t/ydb/databases)
    * [Продовая база /ru/alice/prod/notificator](https://yc.yandex-team.ru/folders/foodpffaf31u2gv7s37t/ydb/databases/etn009i89da1vab1nb1p/overview)
    * [База для \*-test сервисов /ru-prestable/alice/test/notificator](https://yc.yandex-team.ru/folders/foodpffaf31u2gv7s37t/ydb/databases/etn01st9ge57d84pi1ke/overview)
    * [База для \*-dev сервисов /ru-prestable/alice/dev/notificator](https://yc.yandex-team.ru/folders/foodpffaf31u2gv7s37t/ydb/databases/etn00476jf0hpkh6t5pt/overview)

- Old UI

    * [Продовая база /ru/alice/prod/notificator](https://ydb.yandex-team.ru/db/ydb-ru/alice/prod/notificator/browser)
    * [База для \*-test сервисов /ru-prestable/alice/test/notificator](https://ydb.yandex-team.ru/db/ydb-ru-prestable/alice/test/notificator/browser)
    * [База для \*-dev сервисов /ru-prestable/alice/dev/notificator](https://ydb.yandex-team.ru/db/ydb-ru-prestable/alice/dev/notificator/browser)

{% endlist %}

## Балансеры {#balancers}

* [notificator.alice.yandex.net](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/notificator.alice.yandex.net/show)
    * [Веса в l7heavy](https://nanny.yandex-team.ru/ui/#/l7heavy/notificator.alice.yandex.net)
* [notificator-dev.alice.yandex.net](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/notificator-dev.alice.yandex.net/show)
* [notificator-test.alice.yandex.net](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/notificator-test.alice.yandex.net/show)

## Реакции в reactor {#reactor}

* [Reactor project](https://reactor.yandex-team.ru/browse?selected=8916865)

## Релизы {#releases}

* [Релизная машина](https://rm.z.yandex-team.ru/component/matrix/manage)
* [Релизы в Arcadia CI](https://a.yandex-team.ru/projects/speechkit_ops_alice_notificator/ci/releases/timeline?dir=alice%2Fmatrix%2Fnotificator&id=release_matrix)

## Код в аркадии {#code}

* [Новый cpp код](https://a.yandex-team.ru/arc/trunk/arcadia/alice/matrix)
* Старый python код
    * [Код ручек](https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/notificator)
    * [Код запросов в ydb](https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/ydbs)
* API
    * [Все новое API](https://a.yandex-team.ru/arc/trunk/arcadia/alice/protos/api/matrix)
    * API доставшееся по наследству от python кода
        * [Основное место](https://a.yandex-team.ru/arc/trunk/arcadia/alice/protos/api/notificator)
        * [То, что надо перенести](https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/protos/notificator.proto)
