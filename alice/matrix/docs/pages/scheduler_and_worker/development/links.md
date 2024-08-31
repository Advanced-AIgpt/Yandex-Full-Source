# Ссылки для разработчиков/дежурных

## Мониторинг {#monitoring}

### Self monitoring {#self_monitoring}

* [Проект в solomon](https://solomon.yandex-team.ru/admin/projects/matrix)
* [Проект в monitoring](https://monitoring.yandex-team.ru/projects/matrix)
* [Проверки в juggler](https://juggler.yandex-team.ru/project/matrix/dashboard?project=matrix)

## Сервисы в няне {#services}

### Prod {#services_prod}

{% list tabs %}

- Scheduler

    * [matrix-scheduler-prestable](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-scheduler-prestable)
    * [matrix-scheduler-sas](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-scheduler-sas)
    * [matrix-scheduler-man](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-scheduler-man)
    * [matrix-scheduler-vla](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-scheduler-vla)

- Worker

    * [matrix-worker-prestable](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-worker-prestable)
    * [matrix-worker-sas](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-worker-sas)
    * [matrix-worker-man](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-worker-man)
    * [matrix-worker-vla](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-worker-vla)

{% endlist %}

### Testing/dev/etc {#services_unstable}

{% list tabs %}

- Scheduler

    * [matrix-scheduler-test](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-scheduler-test)

- Worker

    * [matrix-worker-test](https://nanny.yandex-team.ru/ui/#/services/catalog/matrix-worker-test)

{% endlist %}

### Дашборды {#services_dashboards}

* [Scheduler](https://nanny.yandex-team.ru/ui/#/services/dashboards/catalog/matrix-scheduler)
* [Worker](https://nanny.yandex-team.ru/ui/#/services/dashboards/catalog/matrix-worker)

## Базы в YDB {#ydb}

{% list tabs %}

- New UI

    * [Все базы вместе](https://yc.yandex-team.ru/folders/foodpffaf31u2gv7s37t/ydb/databases)
    * [Продовая база /ru/speechkit_ops_alice_notificator/prod/matrix-queue-common](https://yc.yandex-team.ru/folders/foodpffaf31u2gv7s37t/ydb/databases/c0f5k9rla5gclrc4fptf/overview)
    * [База для \*-test сервисов /ru-prestable/speechkit_ops_alice_notificator/test/matrix-queue-common](https://yc.yandex-team.ru/folders/foodpffaf31u2gv7s37t/ydb/databases/c0fd45574l8unh56lg5c/overview)

- Old UI

    * [Продовая база /ru/speechkit_ops_alice_notificator/prod/matrix-queue-common](https://db.yandex-team.ru/db/ydb-ru/speechkit_ops_alice_notificator/prod/matrix-queue-common/browser)
    * [База для \*-test сервисов /ru-prestable/speechkit_ops_alice_notificator/test/matrix-queue-common](https://db.yandex-team.ru/db/ydb-ru-prestable/speechkit_ops_alice_notificator/test/matrix-queue-common/browser)

{% endlist %}


## Балансеры {#balancers}

* [matrix-scheduler.alice.yandex.net](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/matrix-scheduler.alice.yandex.net/show)
    * [Веса в l7heavy](https://nanny.yandex-team.ru/ui/#/l7heavy/matrix-scheduler.alice.yandex.net)
* [matrix-scheduler-test.alice.yandex.net](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/matrix-scheduler-test.alice.yandex.net/show)

## Релизы {#releases}

{% list tabs %}

- Scheduler

    * [Релизная машина](https://rm.z.yandex-team.ru/component/matrix_scheduler/manage)
    * [Релизы в Arcadia CI](https://a.yandex-team.ru/projects/speechkit_ops_alice_notificator/ci/releases/timeline?dir=alice/matrix/scheduler&id=release_matrix_scheduler)

- Worker

    * [Релизная машина](https://rm.z.yandex-team.ru/component/matrix_worker/manage)
    * [Релизы в Arcadia CI](https://a.yandex-team.ru/projects/speechkit_ops_alice_notificator/ci/releases/timeline?dir=alice/matrix/worker&id=release_matrix_worker)

{% endlist %}

## Код в аркадии {#code}

* [Весь проект matrix](https://a.yandex-team.ru/arc/trunk/arcadia/alice/matrix)
    * [Код scheduler](https://a.yandex-team.ru/arc/trunk/arcadia/alice/matrix/scheduler)
    * [Код worker](https://a.yandex-team.ru/arc/trunk/arcadia/alice/matrix/worker)
* [API](https://a.yandex-team.ru/arc/trunk/arcadia/alice/protos/api/matrix)
