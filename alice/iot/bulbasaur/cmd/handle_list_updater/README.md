Скрипт для получения списка актуальных мобильных ручек. Использовался для обновления графика методов UI: https://a.yandex-team.ru/arc/trunk/arcadia/alice/iot/dashboards/solomon/bulbasaur.py?rev=r8583519#L163.

Запуск из alice/iot/bulbasaur:

    YAV_SECRET_ID=<...> YAV_TOKEN=<...> ./cmd/handle_list_updater/handle_list_updater --config config

YAV_TOKEN брать отсюда: https://yav.yandex-team.ru/secret/sec-01de9bv57aewxmqg9kn1br8111/explore/versions (найти секрет с названием vault.token)

YAV_SECRET_ID: sec-01ettg0pss657dt3hrv0nwym16
