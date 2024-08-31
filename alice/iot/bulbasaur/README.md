# <img alt="bulb" src="https://jing.yandex-team.ru/files/mavlyutov/bulb.jpg" width="220" />

# Описание
Основной бэкенд умного дома. Обрабатывает запросы от Алисы, UI и сервиса коллбеков Steelix.

Внешняя документация на [tech.yandex.ru](https://yandex.ru/dev/dialogs/smart-home/)

[Swagger API](https://s3.mds.yandex.net/iot/infra/swagger/index.html) (в процессе заполнения)

# Платформа
Бекенд в RTC: [nanny](https://nanny.yandex-team.ru/ui/#/services/) -> quasar -> bulbasaur

БД в YDB:
* [ydb:production/iotdb](https://ydb.yandex-team.ru/db/ydb-ru/quasar/production/iotdb/browser)
* [ydb:prestable/iotdb](https://ydb.yandex-team.ru/db/ydb-ru-prestable/quasar/prestable/iotdb/browser)
* [ydb:development/iotdb](https://ydb.yandex-team.ru/db/ydb-ru-prestable/quasar/development/iotdb/browser)

## Продакшен
* Хост `https://iot.quasar.yandex.ru`: торчит в интернет; основной клиент UI; балансер [iot.quasar.yandex.ru](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/iot.quasar.yandex.ru/)
  * `/m` -> [bulbasaur_man](https://nanny.yandex-team.ru/ui/#/services/catalog/bulbasaur_man/), [bulbasaur_vla](https://nanny.yandex-team.ru/ui/#/services/catalog/bulbasaur_vla/), [bulbasaur_sas](https://nanny.yandex-team.ru/ui/#/services/catalog/bulbasaur_sas/)
  * `/t/m` -> proxy `http://tuya.iot.quasar.yandex.net:80/m`
* Хост `http://iot.quasar.yandex.net`: не торчит в интернет; основные клиенты — ASR и MM; балансер [iot.quasar.yandex.net](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/iot.quasar.yandex.net/)
  * `/` -> [bulbasaur_man](https://nanny.yandex-team.ru/ui/#/services/catalog/bulbasaur_man/), [bulbasaur_vla](https://nanny.yandex-team.ru/ui/#/services/catalog/bulbasaur_vla/), [bulbasaur_sas](https://nanny.yandex-team.ru/ui/#/services/catalog/bulbasaur_sas/)
* YDB: [/ru/quasar/production/iotdb](https://ydb.yandex-team.ru/db/ydb-ru/quasar/production/iotdb/info)

## Бета
* Хост `https://iot-beta.quasar.yandex.ru`: торчит в интернет; основной клиент UI; балансер [iot-beta.quasar.yandex.ru](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/iot-beta.quasar.yandex.ru/)
    * `/m` -> [bulbasaur_beta](https://nanny.yandex-team.ru/ui/#/services/catalog/bulbasaur_beta/)
    * `/t/m` -> proxy `http://tuya-beta.iot.quasar.yandex.net:80/m`
* Хост `http://iot-beta.quasar.yandex.net`:  не торчит в интернет; основные клиенты — ASR и MM; балансер [iot-beta.quasar.yandex.net](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/iot-beta.quasar.yandex.net/)
  * `/` -> [bulbasaur_beta](https://nanny.yandex-team.ru/ui/#/services/catalog/bulbasaur_beta/)
* YDB: [/ru-prestable/quasar/prestable/iotdb](https://ydb.yandex-team.ru/db/ydb-ru-prestable/quasar/prestable/iotdb/info)

## Дев-стенд
* Хост `https://iot-dev.quasar.yandex.ru`: не торчит в интернет; основной клиент UI; балансер [iot-dev.quasar.yandex.ru](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/iot-dev.quasar.yandex.ru/)
  * `/m` -> [quasar-iot-dev](https://nanny.yandex-team.ru/ui/#/services/catalog/quasar-iot-dev/)
  * `/t/m` -> [quasar-tuya-adapter](https://nanny.yandex-team.ru/ui/#/services/catalog/quasar-tuya-adapter/)
* Хост `http://iot-dev.quasar.yandex.net`: не торчит в интернет; основные клиенты - ASR и MM; балансер [iot-dev.quasar.yandex.ru](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/iot-dev.quasar.yandex.ru/)
  * `/` -> [quasar-iot-dev](https://nanny.yandex-team.ru/ui/#/services/catalog/quasar-iot-dev/)
* YDB: [/ru-prestable/quasar/development/iotdb](https://ydb.yandex-team.ru/db/ydb-ru-prestable/quasar/development/iotdb/browser)

# Разработка
__(небольшая компиляция стартовых полезных знаний при разработке в Аркадии)__

## Кодогенерация и IDE, go build, go test...
Стандартный тулинг не поддерживает кодогенерацию, описанную в `ya.make`. Поэтому IDE может не резолвить методы или типы;
go test будет ругаться на зависимости и т.п. При сборке через `ya make` все эти зависимости генерируются, но остаются
в кэше. Чтобы явно добавить сгенерированный код в репозиторий, достаточно запустить
сборку через `ya make` с ключом `--add-result go`:
```(sh)
ya make --add-result go
```
Указывать этот ключ нужно только в первый раз; по крайней мере до тех пор, пока не появятся новые зависимости.

## arc: игнорируем лишнее
Кодогенерация, результаты тестов: `~/.arcignore.symlink`
```
**
```

## Просто собрать бинарь
```(sh)
cd $ARCADIA/alice/iot/bulbasaur/cmd/server/
ya make
```
в папке появится бинарь ```bulbasaur```

## Запуск тестов
Интеграционные тесты запускаются командой:
```(sh)
ya make -tt $ARCADIA/alice/iot/bulbasaur
```
Интеграционные тесты поднимают локальную YDB через [официальный рецепт](https://a.yandex-team.ru/arc/trunk/arcadia/kikimr/public/tools/ydb_recipe).
Важно! Для запуска на маке в файл ```/etc/hosts``` должна быть добавлена запись ```127.0.0.1 <your_hostname>```. Хостнейм можно узнать через команду ```hostname -f```
Без этой записи тесты будут падать по ctx deadline на этапе ```SetupDB```.

## Генерация
### Swagger
Генерация swagger-спеки через go generate
```sh
cd server/swagger
ya tool go generate
```

# Сборка образа

## Автоматическая на каждый коммит
* Таска в TestEnv: [QUASAR_IOT_BULBASAUR_DOCKER_BUILD](https://testenv.yandex-team.ru/?screen=job_history&database=alice-iot&job_name=QUASAR_IOT_BULBASAUR_DOCKER_BUILD)
* Исходники: [/arc/trunk/arcadia/testenv/jobs/quasar/BulbasaurBuild.py](https://a.yandex-team.ru/arc/trunk/arcadia/testenv/jobs/quasar/BulbasaurBuild.py)

## Локальная разработка
1. Сохраняем секрет TVM-приложения [bulbasaur](https://abc.yandex-team.ru/services/alice_iot/resources/?show-resource=8448717) в файл `$HOME/.tvm/2009295.secret`
2. Запускаем `tvm-tool` при помощи скрипта `./misc/local/tvm.sh`
3. Запускаем `bulbasaur`:
   - выставляем `ENV_TYPE=DEVELOPMENT`, чтобы подключился конфиг для локальной разработки,
   - указываем путь к папке с конфигами
   ```(sh)
   cd $ARCADIA/alice/iot/bulbasaur
   ENV_TYPE=DEVELOPMENT ./cmd/server/bulbasaur -C ./config
   ```

## Ручная
Сборка осуществляется с помощью `ya package`, который на самом деле сперва запускает `ya make`, а затем создает окружение для сборки контейнера, используя описание из `pkg.json`.
```(sh)
cd $ARCADIA/alice/iot/bulbasaur
ya package pkg.json --docker --docker-repository=iot --target-platform=DEFAULT-LINUX-X86_64
```
Будет собран образ `registry.yandex.net/alice/iot/bulbasaur:{revision}`, где `{revision}` — текущая ревизия локального кода

Если нужно сразу запушить, то надо добавить параметр `--docker-push`:
```
cd $ARCADIA/alice/iot/bulbasaur
ya package pkg.json --docker --docker-repository=iot --docker-push --target-platform=DEFAULT-LINUX-X86_64
```

Чтобы выкатить собранный образ нужно запушить его в репозиторий (по желанию можно добавить тег с описанием образа):
```(sh)
docker tag registry.yandex.net/alice/iot/bulbasaur:{revision} registry.yandex.net/alice/iot/bulbasaur:{revision}-{description}
docker push registry.yandex.net/alice/iot/bulbasaur:{revision}-{description}
```
Здесь `{description}` — ключ или описание задачи, например `IOT-1-v1`

Если хотите запускать всё полностью через докер в QYP локально, не создавая образов в registry.yandex.net
Prerequisite:
- docker
- Тачка в QYP с нашими сетями _GENCFG_QUASAR_IOT_, с NAT64 и включенным ipv4/ipv6 port forwarding
- Выставленные переменные среды в QYP с именами как в nanny
- Следующие bash-команды
```(shell)
cd arcadia/alice/iot/bulbasaur
ya package pkg.json --docker --docker-repository=iot --docker-network=host --target-platform=DEFAULT-LINUX-X86_64 --custom-version=development
docker run -e ENV_TYPE=$ENV_TYPE -e YAV_SECRET_ID=$YAV_SECRET_ID -e YAV_TOKEN=$YAV_TOKEN -e TVM_TOKEN=$TVM_TOKEN -e TVM_SECRET=$TVM_SECRET -e PUSH_CLIENT_TVM_SECRET=$PUSH_CLIENT_TVM_SECRET registry.yandex.net/iot/bulbasaur:development 
```
На виртуалке можно использовать параметр `--docker-network=host`, чтобы докер использовал сеть хоста, на котором запущен
Это поднимет в вашем терминале supervisord и вы будете видеть его output
Чтобы почитать другие логи, можете выполнить в соседней коносли аттач к контейнеру и чтение логов:
```(shell)
docker exec -ti {YOUR_CONTAINER_ID} tail -f /logs/bulbasaur.out
```
например, вот так:
```(shell)
docker exec -ti $(docker ps -q -f "ancestor=registry.yandex.net/iot/bulbasaur:development") tail -f /logs/bulbasaur.out
```

# Патчи к релизу
## Через релизную машину (предпочтительный вариант)
На старанице https://rm.z.yandex-team.ru/component/bulbasaur/merge_rollback указать ветку для которой нужен патч и перечислить
добавляемые коммиты.

## Если первый вариант не работает (например есть мердж-конфликты)
1. Сделать чекаут текущей релизной ветки
```bash
arc checkout -b stable-106 releases/bulbasaur/stable-106
```

2. Сделать нужные изменения, поресолвить конфликты
```bash
arc cherry-pick r8623197
```

3. Запушить изменения в релизную ветку
   после этого релизная машина подцепляет изменения, для каждого нового коммита в ветке создаёт релизный тег
   и выкатывает изменения на бета-стенд.
```
arc push
```
В релизной машине есть механизм разрешения гонок и на бете всегда окажется самая свежая версия,
без откатов и гонок, но сборка образа будет выполняться столько раз сколько коммитов было запушено

От пуша в ветку до создания нового тега в релизной машине может проходить 15-20 минут.

# Роботы и зомбики
Для смены доменных паролей необходимо получить роль "Управляющий роботами" [в ABC](https://abc.yandex-team.ru/services/vsdev/); для просмотра — роль "Пользователь роботов".

* [Завр Бульба](https://staff.yandex-team.ru/robot-bulbasaur) aka Bulbasaur — главный по автоматизации процессов УД. Сборка, деплой, бэкапы и прочее взаимодействие с API через "пользовательские" токены.
  * Пароль от [robot-bulbasaur@yandex-team.ru](https://yav.yandex-team.ru/secret/sec-01de9bv57aewxmqg9kn1br8111/explore/versions)

* [Иви Бульба](https://staff.yandex-team.ru/zomb-eevee) — пользователь УД в окружении Бета. ~~Есть телефон с аккаунтом и набором устройств.~~
  * Пароль от [zomb-eevee@yandex-team.ru](https://yav.yandex-team.ru/secret/sec-01dcs0jxy727cdc3vjewtd0zjq/explore/versions)
  * Пароль от [yndx-zomb-eevee@yandex.ru](https://yav.yandex-team.ru/secret/sec-01dd83yyn96jf5cm26ddcsm1em/explore/versions)

* [Сквиртл Бульба](https://staff.yandex-team.ru/zomb-squirtle) — пользователь УД в окружении Дев. ~~Есть телефон с аккаунтом и набором устройств.~~
  * Пароль от [zomb-squirtle@yandex-team.ru](https://yav.yandex-team.ru/secret/sec-01dcrwbasccnkdcnzgr8r65bg6/explore/versions)
  * Пароль от [yndx-zomb-squirtle@yandex.ru](https://yav.yandex-team.ru/secret/sec-01dd83csfewsve1q1hmq58969m/explore/versions)

# Полезные запросы в логи/базу
**Продакшен-базу не следует нагружать аналитикой или любыми неотлаженными запросами.**
Для аналитики надо использовать копию базы в YT; для отладки и оптимизации запросов — дев- или бета-базу.

У нас есть два основных источника данных: логи и база.

## Логи
Хранятся [в YT](https://yt.yandex-team.ru/arnold/navigation?path=//home/logfeller/logs&filter=vsdev), разбиты по источникам.
Наибольший интерес представляют логи основного приложения
[bulbasaur-production-logs](https://yt.yandex-team.ru/arnold/navigation?path=//home/logfeller/logs/vsdev-bulbasaur-production-logs)
и логи балансера (access-log)
[balancer-iot](https://yt.yandex-team.ru/arnold/navigation?path=//home/logfeller/logs/vsdev-balancer-iot)

## БД: устройства, группы, комнаты, сценарии и всё-всё-всё
Хранится в YDB.

Quick tip: быстро получить user_id пользователя по логину можно по ссылке https://passport.yandex.ru/account/short?login=\<login\>

## Окружения и миграции

В разных окружениях используются разные инстансы базы в ydb:

Кластер | Префикс | Описание
--- | --- | ---
ydb-ru | /ydb-ru/quasar/production/iotdb | Продовая база, только для работы сервиса. Ручные запросы запрещены во избежание локов базы, которые аффектят продакшен.
ydb-ru-prestable | /ydb-ru-prestable/quasar/beta/iotdb | Бета база - отдельная база для QA, не продовые данные.
ydb-ru-prestable | /ydb-ru-prestable/quasar/backup/iotdb | Бекап база, на которую раз в сутки накатывается бекап прода. Можно использовать для читающих ручных запросов.
ydb-ru-prestable | /ydb-ru-prestable/quasar/development/iotdb | База для dev окружения
ydb-ru-prestable | /ydb-ru-prestable/quasar/test/iotdb | База для тестового окружения - отдельная база, которая по схеме повторяет прод, но со своими данными. Используется для тестирования смежными командами.
ydb-ru-prestable | /ydb-ru-prestable/quasar/prestable/iotdb | Старая бета база, не используется. Можно использовать для проведения нагрузочного тестирования и стрельб.

Порядок накатывания миграций

Т.к. миграции всегда пишутся так, чтобы не сломать текущий работающий код, то порядок выкатки обычно следующий:
1) перед тем как выкатить релиз на dev и beta надо выкатить миграцию на dev и beta соответственно
2) далее проходит регресс
3) перед тем как выкатить релиз в продакшен надо выкатить миграцию на prod базу и test базу
4) далее катим релиз

## Копия БД в YT
Данные хранятся [в YT](https://yt.yandex-team.ru/hahn/navigation?path=//home/iot/backup/bulbasaur-ydb); копируются
из продакшен-базы каждую ночь ([sandbox-scheduler](https://sandbox.yandex-team.ru/scheduler/43968)). Отлично подходят
для аналитических запросов, в т.ч. регулярных.

При необходимости, [любой бэкап](https://sandbox.yandex-team.ru/resources?type=YDB_BACKUP_DATA&attrs=%7B"ydb_database"%3A"%2Fru%2Fquasar%2Fproduction%2Fiotdb"%7D)
можно развернуть в YT для последующей аналитики при помощи sandbox-таски `YDB_RESTORE_TO_YT`.

Чтобы развернуть разработческую базу из YT-копии нужно запустить такой запрос, предварительно заменив в нем ```$ydb_database``` и ```$ydb_endpoint```,
[YT_TO_YDB_RESTORE](https://yql.yandex-team.ru/Operations/YGSLvgPTTrrBgBHOcJ9xd2ZgOkhFSZif_9XAEam1FJ4=)

Также есть возможность скопировать не всю базу целиком, а только одного пользователя следующим запросом: [YT_TO_YDB_RESTORE_ONE_USER](https://yql.yandex-team.ru/Operations/X3tXMpdg8i8VWBb0fmOaU90AIpIKePt_7iob_9aK2eo=). Для этого нужно указать user_id в поле ```$user_id``` и базу, в которую мы хотим скопировать пользователя в полях ```$ydb_database```,  ```$ydb_endpoint```.

Если база новая, то перед копированием нужно создать схему:
```
cd alice/iot/bulbasaur/cmd/db_schema/
YDB_ENDPOINT=ydb-ru-prestable.yandex.net:2135 YDB_PREFIX=/ru-prestable/home/<username>/mydb ./db_schema
```

## Построение вторичного индекса в YDB

На примере таблицы Devices, нужно было построить индекс по полю external_id. С помощью утилиты `ya ydb` можно это сделать так:
```
ya ydb -e ydb-ru-prestable.yandex.net:2135 -d /ru-prestable/quasar/development/iotdb table index add global --index-name devices_external_id_index --columns external_id Devices
```
Тут `devices_external_id_index` - это имя индекса (хорошее правило именования - включить имя таблицы и колонки, по которым этот индекс построен).
Процесс будет запущен в фоне, посмотреть прогресс можно с помощью такой команды:
```
ya ydb -e ydb-ru-prestable.yandex.net:2135 -d /ru-prestable/quasar/development/iotdb operation list buildindex
```

После завершения процесса полезно посмотреть description таблицы, чтобы убедиться, что индекс есть:
```
ya ydb -e ydb-ru-prestable.yandex.net:2135 -d /ru-prestable/quasar/development/iotdb scheme describe Devices
```

Ну и еще полезно прогнать свой запрос и посмотреть на query plan, чтобы убедиться, что индекс используется для запроса корректно:
```
ya ydb -e ydb-ru-prestable.yandex.net:2135 -d /ru-prestable/quasar/development/iotdb table query explain -q "PRAGMA TablePathPrefix('/ru-prestable/quasar/development/iotdb'); SELECT * FROM Devices : devices_external_id_index WHERE external_id=='24307894c12010120510.yandexstation'"
```
По плану можно убедиться в отсутствии FullScan операций, и там же видно, что индекс реально используется (сейчас видно, что есть отдельный запрос в таблицу с индексом).

Документацию по процессу и рекомендаци можно найти по ссылкам:
[Работа с утилитой ya ydb](https://ydb.yandex-team.ru/docs/getting_started/ydb_cli#zapusk-operacii-dobavleniya-vtorichnogo-indeksa)
[Рекомендации по вторичным индексам](https://ydb.yandex-team.ru/docs/best_practices/secondary_indexes)

## Примеры
[кол-во пользователей](https://yql.yandex-team.ru/Operations/XTsjTDa9vPdXXD6iGKV34qLRq9EmO82aTSt_yDipy2s=)

[кол-во пользователей УД, которые имеют колонку](https://yql.yandex-team.ru/Operations/X0eoSWim9cjnhsl_jiXYvGkCB5D5jPZWAkudJKa-Or0=)

[кол-во пользователей в разбивке по дням](https://yql.yandex-team.ru/Operations/XTtlvWim9eC54V0KyYHEy8g4Kx_Ld7F_Ca9bz3q8rYA=)

[логи за последний час](https://yql.yandex-team.ru/Operations/XTtjPmim9eC54VwtY868JSGrTABPNM6RpCMG1lwQHak=)

коды ответов с разбивкой по ручкам и времени: [свежие логи (~1 сутки)](https://yql.yandex-team.ru/Operations/XYCaKlPzVFPYvDSgWfvEqMrHaQTCy6uEILh9bXli1qY=)

[топовые названия Девайсов, Сценариев, Комнат, Групп](https://yql.yandex-team.ru/Operations/XTtoa59LnqCUGD6FZnFYvKgRxn0jE0ExdROUrNMRXYw=)

[уникальные девайсы Xiaomi, которые за последний месяц не прошли whitelist](https://yql.yandex-team.ru/Operations/XTtnKAlcTuAZ9B1IAO-vL6yfYn0xG3RMe8iaX81tcq0=)

кол-во запросов пользователей в разбивке по дням за последний месяц: [голосом](https://yql.yandex-team.ru/Operations/XTtnfja9vPdXXFe6H2gI_9PkE7d6Ef4-yMB_7uY5RxI=), [ПП](https://yql.yandex-team.ru/Operations/XUCIS2im9WUDlexSmehFTAadpIJCGItWFp_3f3vm5Wo=)

запросы конкретного пользователя за последние: [7 дней](https://yql.yandex-team.ru/Operations/XTtvfGHljsp26f3Tffut4H-nWxsLbNHYx76slXCHRfc=), [1 час](https://yql.yandex-team.ru/Operations/XT4CqmHljiENezgysG8z5KwcOLbaX8jCB8ZzfzZDyu4=)

запросы в конкретный skill_id за последние: [7 дней](https://yql.yandex-team.ru/Operations/Xus6LiAsJfBz134GdCQJjvfC6odOaRQ_GXcmc9qM5gE=), [1 час](https://yql.yandex-team.ru/Operations/Xus5TCAsJfBz13188vccrVb2yCsY1U-r4z46JzngD2U=)

запросы про конкретный девайс за последние: [7 дней](https://yql.yandex-team.ru/Operations/X8TAnQPTTgcXSgBYDcnWmlAUpAVsumdj9f_oq2eaDx8=)

девайсы, сменившие тип: [девайсы](https://yql.yandex-team.ru/Operations/XT7zeZ9LnnjvdhGjt6I5FdTV4XUQJp60kcyPnZZqoIU=), [уники](https://yql.yandex-team.ru/Operations/XT7z9mim9WUDlUan-bhSgBQblvzWyh99yGB_uphEvC0=)

ошибки discovery в разрезе навыков: [7 дней](https://yql.yandex-team.ru/Operations/XrJhAmim9Qn5dWViaJb0iOs2EmpBnBsf7YD-7Ni9oPk=), [1 час](https://yql.yandex-team.ru/Operations/XrJkOmHljlEOqwROz39FUPRIXOt4r_lfHLwWnCdb-ac=)

[кол-во девайсов по брендам](https://yql.yandex-team.ru/Operations/XX9-21PzVFPYu94UHWFBPtDA5TGgnxWV2AtAeTfzvXU=)

[кол-во устройств в разбивке по моделям](https://yql.yandex-team.ru/Operations/XVGeFwlcTpHZg7YXAurjoObHVOljeatBe9uYJq6Qtcg=)

[кол-во уникальных моделей устройств](https://yql.yandex-team.ru/Operations/XmHzXGHljoub239PzfQjFRUzsR6BKA-He_sbT9Fo90A=)

[уникальные модели в разрезе типов устройств](https://yql.yandex-team.ru/Operations/XmkWmZ9Lnvf0XfbRHiuFpOREHfkGvJ-KDHlfsir8iM0=)

уникальные модели в разбивке по производителям: [ydb](https://yql.yandex-team.ru/Operations/XoOpSwtcP1IM6d6tpen4Uxt0ZxIa8HtRZhx6k-AkUUQ=), [yt](https://yql.yandex-team.ru/Operations/XoOsLFPzVCXwhdfltOzU24vCrLpHYZDaXRMI9Z7Kl78=)

[кол-во пользователей УД, у которых есть хотя бы одна колонка](https://yql.yandex-team.ru/Operations/XVWXEGim9TvFwpp0JKbvmFv23FoVbQRkeF9vsSvprq0=)

статисика качества бренда за последние: [7 дней](https://yql.yandex-team.ru/Operations/Xe5m9lPzVBW8UJP6-JxrPg17gmMCRTBlElEy02BjeM8=)

[кол-во устройств провайдера в разрезе по типам](https://yql.yandex-team.ru/Operations/XfDmR1PzVBW8UYCQOBRjjyAdtMwSeurgEVQRmvKclJc=)

[ошибки получения токена по application name](https://yql.yandex-team.ru/Operations/XfkY4WHljoNLZa_WXjyx0znkLbIoM2BCbbnpY8eotLQ=)

[распределение кол-ва колонок по пользователям](https://yql.yandex-team.ru/Operations/Xfu0Kmim9ZaSIDk_RnU-RkdDGMGv_NwKlJwTE1U8Bag=)

[навыки, которые не заполняют model/manufacturer](https://yql.yandex-team.ru/Operations/Xn4dTGHljnLT2Iv1sgkffEfyoHnFgsxBTwERwwxfX2k=)

[кол-во созданных отложенных сценариев, сгруппированных по дням](https://yql.yandex-team.ru/Operations/X38HeJ3udnJlkW93OVpizbAQcXcDVozAGnt1q6xcsMo=)
