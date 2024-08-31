# Бэкэнд-сервис для приёма платежей в Яндекс.Станции и Яндекс.Модуле

## Подробное описание:
https://wiki.yandex-team.ru/quasar/dev/Billing/

## Дока по API:
https://wiki.yandex-team.ru/quasar/dev/Billing/quasar-billing-API/

## CI и процессы

CI-Джобы тут: https://cauldron.yandex-team.ru/quasar-dev/quasar-billing

Prod тут: https://platform.yandex-team.ru/projects/x-products/quasar-backend/main-prod
Test тут: https://platform.yandex-team.ru/projects/x-products/quasar-backend/main

CI собирает образа `quasar-billing/prod:{branch}-{commit}` и `.../test:...` на каждый коммит.

По тегу в мастере собирается образ с тегом -- например, `quasar-billing/prod:0.1.3`, который и нужно, пока руками, выложить куда следует.

Текущий флоу примерно такой:
* отвести фича-ветку
* накодить
* поднять личную демку и поиграться там
* пройти ревью, смержить
* создать релиз на Github
* дождаться пуша тэга
* прогнать вручную sql-ные upgrade-скрипты из `/src/main/resources/create.sql` на test-овой базе
* выложить тэгнутый контейнер на test
* проверить работоспособность на test-е:
    * взять тестовый аккаунт с привязанной на test-е картой из https://wiki.yandex-team.ru/quasar/test-accounts/
    * подёргать под ним GET-ручки (pricingOptions, availabilityInfoBatch, getCardsList, getContentMetaInfo и т.д.)
    * купить или взять в аренду под этим аккаунтом что-нибудь на ivi (с ivi у нас по test-у рассчётов не идёт) -
      по-настоящему через колонку
* прогнать вручную sql-ные upgrade-скрипты из `/src/main/resources/create.sql` на prod-овой базе
* выложить тэгнутый контейнер на прод
* проверить работоспособность на prod-е:
  * взять свой prod-овый аккаунт с привязанной живой карточкой
  * подёргать под ним GET-ручки (pricingOptions, availabilityInfoBatch, getCardsList, getContentMetaInfo и т.д.)
  * купить или взять в аренду под этим аккаунтом что-нибудь на ivi (с ivi у нас по договору идёт удержание refund-ов из наших платежей к ним) - по-настоящему через колонку
  * сделать вручную возврат платежа через API trust-а - https://wiki.yandex-team.ru/TRUST/Payments/API/Refunds/


## Локально

Прогнать тесты и сборку, *как в CI* -- `./drone_local.sh`. Нужен настроенный докер.

Прогнать тесты *как обычно* -- `./gradlew clean test`.

Собрать прод-версию контейнера локально -- `docker build --target prod . -t registry.yandex.net/quasar-billing/my_fancy_name:1`.

Собрать тест-версию контейнера локально -- `docker build --target test . -t registry.yandex.net/quasar-billing/my_fancy_name:1`.

```docker build --target release -t "registry.yandex.net/quasar-billing:`git name-rev --name-only HEAD`-`git rev-parse --short HEAD`" .```

Пушнуть -- как у всех, `docker push registry.yandex.net/quasar-billing/my_fancy_name:1`

Запустить локально -- `TO BE IMPLEMENTED`
