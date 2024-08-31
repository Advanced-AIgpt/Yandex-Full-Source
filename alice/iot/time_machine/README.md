# Описание

Сервис для отложенного выполнения задач. Задача - вызов определенного http запроса с произвольным телом в заданное время. Поддерживает ретраи (если сервис ответил не 2хх).

В умном доме используется следующим образом: bulbasaur при запросе, в котором есть гипотеза с указанным временем, создает сценарий и создает задачу в машине времени на выполнение этого сценария в указанное время. Ручка в бульбазавре используется такая: ```POST http://<bulbasaur_host>/time_machine/scenarios/{scenarioId}/invoke```.

# Ручки

1. Постановка задачи:
```POST /v1.0/submit```

Тело запроса (пример):
```(json)
{
  "schedule_time": "2020-10-21T10:37:45.089924Z",  // время в UTC, на которое мы хотим запланировать вызов колбека
  "http_method": "POST",  // http метод (GET/POST/PUT), с которым будет вызван колбек
  "url": "http://iot.quasar.yandex.net/time_machine/scenarios/{scenarioId}/invoke",  // url колбека
  "headers": {
    "Header1": "Value1",
    "Header2": "Value2"
  },  // хедеры, с которыми будет вызван колбек
  "request_body": "SGVsbG8=",  // base64 закодированное тело, с которым будет вызван колбек
  "service_tvm_id": 123321,  // tvm id сервиса, в котором будет вызван колбек
  "user_id": 100500  // идентификатор пользователя, для которого был создан колбек (см. пояснение)
}
```

Пояснения:
* user_id в теле запроса - это ключ, с помощью которого задачи на вызов колбека группируются внутри time machine. Сейчас он пока не используется (для time machine, но используется для другого сервиса, построенного на той же библиотеке для реализации очереди), но потенциально будет использоваться для шардирования, поэтому лучше передавать там что-то осмысленное, вроде идентификатора пользователя, которому принадлежит задача.
* к списку хедеров перед вызовом будет добавлены 2 служебных: X-Ya-Service-Ticket, в котором будет сервисный tvm тикет, и X-Ya-User-ID, в котором будет user_id, переданный в запросе
* колбек будет выполняться до тех пор, пока не получит в ответ код 2хх. Ретраи устроены следующим образом: 5 ретраев каждые 30 секунд, после этого 20 ретраев с линейно увеличивающимся интервалом (сначала через 5 минут, потом через 10, 15, 20 и т.д.) - тут следует опасаться возможных проблем вида "если сервер стал пятисотить под нагрузкой, то ретраи добивают сервис увеличенной нагрузкой". Сейчас ретраи подобраны так, чтобы колбек точно выполнился, но из-за того, что самих колбеков в сервис УД не много, то пока это не проблема. В будущем, возможно, надо сделать так, чтобы можно было задавать политику ретраев в запросе.

Важно:
* для того, чтобы звать колбеки в каком-то сервисе нужно, чтобы time machine могла выписать для него сервисный тикет, для этого его надо прописать в список dst в скрипте tvm.sh

# Графики

Service stats: https://solomon.yandex-team.ru/?project=alice-iot&cluster=timemachine_production&service=timemachine&dashboard=timemachine_service_dashboard&l.host=cluster

# Платформа
Бекенд в RTC: [nanny](https://nanny.yandex-team.ru/ui/#/services/) -> iot -> time_machine

БД в MDB:
* [pg:production/time_machine_prod_db](https://yc.yandex-team.ru/folders/foojfj9s7o74c15qrssv/managed-postgresql/cluster/mdbvtjbrb474l1mds0du)
* [pg:development/time_machine_dev_db](https://yc.yandex-team.ru/folders/foojfj9s7o74c15qrssv/managed-postgresql/cluster/mdbq2fmkrsos1ea46vat)

## Продакшен
* TVM: service id сервиса `2021514`
* Балансер `http://timemachine.iot.yandex.net` [vla](https://nanny.yandex-team.ru/ui/#/services/catalog/rtc_balancer_timemachine_iot_yandex_net_vla/), [man](https://nanny.yandex-team.ru/ui/#/services/catalog/rtc_balancer_timemachine_iot_yandex_net_man/), [sas](https://nanny.yandex-team.ru/ui/#/services/catalog/rtc_balancer_timemachine_iot_yandex_net_sas/)
* Сервис [vla](https://nanny.yandex-team.ru/ui/#/services/catalog/timemachine_vla/), [man](https://nanny.yandex-team.ru/ui/#/services/catalog/timemachine_man/), [sas](https://nanny.yandex-team.ru/ui/#/services/catalog/timemachine_sas/)

## Бета-стенд
* TVM: service id сервиса `2032560`
* Общий балансер: `timemachine-beta.iot.yandex.net`, используется общий [beta-балансер](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/beta.iot.yandex.net/show)
* Бета бульбазавра смотрит в бету тайм-машины

# Разработка

Для сервиса написана общая либа, реализующая очередь задач https://a.yandex-team.ru/arc/trunk/arcadia/alice/library/go/queue
Либа поддерживает большую функциональность, чем используется машиной времени, например:
* есть возможность сделать повторяющиеся таски
* можно указать merge policy для тасков (задать таску merge key и политику, что делать, если в базе уже есть таск, готовый к выполнению с таким же merge key, как у новой таски - можно апдейтить параметры выполнения у предыдущей таски или завершать постановку с ошибкой, или просто не ставить новый таск и оставлять старый в очереди)

# Сборка образа

## Ручная
Сборка осуществляется с помощью `ya package`, который на самом деле сперва запускает `ya make`, а затем создает окружение для сборки контейнера, используя описание из `pkg.json`.
```(sh)
cd ~/arcadia/alice/iot/time_machine
ya package pkg.json --docker --docker-repository=iot --target-platform=DEFAULT-LINUX-X86_64
```
Будет собран образ `registry.yandex.net/alice/iot/timemachine:{revision}`, где `{revision}` — текущая ревизия локального кода

Если нужно сразу запушить, то надо добавить параметр `--docker-push`:
```
cd ~/arcadia/alice/iot/time_machine
ya package pkg.json --docker --docker-repository=iot --docker-push --target-platform=DEFAULT-LINUX-X86_64
```

Чтобы выкатить собранный образ, нужно дать ему уникальный тег и запушить его:
```(sh)
docker tag registry.yandex.net/alice/iot/timemachine:{revision} registry.yandex.net/alice/iot/timemachine:{revision}-{description}
docker push registry.yandex.net/alice/iot/timemachine:{revision}-{description}
```
Здесь `{description}` — ключ или описание задачи, например `IOT-1-v1`

# Логи
Хранятся [в YT](https://yt.yandex-team.ru/arnold/navigation?path=//home/logfeller/logs&filter=vsdev).
* Продовые логи [timemachine-production-logs](https://yt.yandex-team.ru/arnold/navigation?path=//home/logfeller/logs/vsdev-timemachine-production-logs)
* Бета логи [timemachine-production-logs](https://yt.yandex-team.ru/arnold/navigation?path=//home/logfeller/logs/vsdev-timemachine-beta-logs)

# БД

База в MDB.
- Имя базы: time_machine_db
- Админский пользователь: iot (имеет права на изменение-создание таблиц, от его имени нужно накатывать миграции)
- Пользователь приложения: time_machine (имеет права на схему iot - чтение и запись в таблицы)
- Пароли пользователей хранятся в секретнице: [time_machine_prod_db](https://yav.yandex-team.ru/secret/sec-01emezr1smvw23saka1v4yjb2d/explore/versions) и [time_machine_dev_db](https://yav.yandex-team.ru/secret/sec-01emezv9h2tbdpdf1z74wx6dye/explore/versions)

К базе можно подключиться с помощью pslq так:
```
psql 'postgresql://time_machine:<password>@<cluster-host>.db.yandex.net:6432/time_machine_db'
```

## Миграция базы

Миграции в постгресе можно накатывать с помощью удобной утилиты pg-migrate, в ней учтено много разных нюансов. Тулза лежит в аркадии и собрать ее можно так:
```
cd arcadia/contrib/python/yandex-pgmigrate/bin
ya make
```

Запустить миграцию до версии 42 можно так:
```
./pgmigrate migrate -c 'host=<cluster_primary_host_name>.db.yandex.net port=6432 dbname=time_machine_db user=iot password=<password>' -d arcadia/alice/library/go/queue/pgbroker/db -t 42 -v -a afterAll:grants
```
