Алерты создаются при помощи библиотеки [solo](https://a.yandex-team.ru/arc/trunk/arcadia/library/python/monitoring/solo).

Для добавления новых сущностей (сервисов, кластеров, шардов, алертов Solomon) нужно добавить соответствующие классы в `registry`.

## Как посмотреть дифф

`ya make && ./creator/creator`

## Как задеплоить алерты

`ya make && ./creator/creator --apply-changes`
