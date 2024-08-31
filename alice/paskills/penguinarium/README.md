# Penguinarium #
Пингвинарий -- это сервис для классификации микро интентов. Основная сущность сервиса -- Нода. Каждой Ноде соответсвует произвольное (> 1) количество Интентов. Каждому интенту соответствует произвольное (> 0) количество фраз. Таким образом для запроса **(node_id, utterance)** сервис сообщает **intent_id** -- интент из указанной ноды, фраза которого наиболее близка к присланной фразе.

Для превращения фразы в веткор используется DSSM модель.
[Статья на Вики](https://wiki.yandex-team.ru/dssm/dataandmodels/). Название модели **lb_insight_qfuf201903_fps_large**. Модель обучалась на парах запрос / расширение запроса.
Для поиска intent_id используется простой KNN (для 1 соседа 1NN соответственно)


## Минимальные действия по сборке и запуску ##

### Для локального запуска (без DOCKER), с Redis ###
1. Из директории **/models** выполнить ```ya make```. Этой командой загрузим DSSM модель из Sandbox
2. Запустить Redis: ```docker run redis -p 6379:6379```
3. Из директории **/app** выполнить ```ya make```.
4. Запустить приложение ```./app```

### Для локального запуска (без DOCKER), без Redis ###
**Внимание! При выключенном Redis ручка /solomon станет отдавать пустой ответ. Если работоспособность этой ручки важна, Redis отключать нельзя!**

Аналогично предыдущей инструкции, за исключением пункта 3.

3. Открыть файл **/app/configs/debug_local/app.yaml**. Убедиться, что выставлена следующая настройка
```yaml
redis:
    turned_on: False
```

### Для сборки в Docker ###
Из директории **/docker** выолнить:
```bash
ya package penguinarium.json --docker
```

Для сборки контейнера + заливки в Docker Registry:
```bash
ya package penguinarium.json --docker --docker-push
```


## Разбор файла app.yaml ##
В этом файле настраиваются все основные свойства приложения настраиваются в этом конфиге.

```yaml
ydb:
    endpoint: -- берутся из вкладки info вашей базы
    database: -- берутся из вкладки info вашей базы
    timeout:  -- таймаут запросо к базе
    max_retries: -- кол-во ретраев на запрос
    root_folder: -- корневая папка, все таблицы приложение будет искать в ней
    connect_params:
        use_tvm: -- использовать ли TVM
        self_tvm_id: -- (необходим только для TVM)
        tvm_secret_env:  -- переменная окружения, в которой лежит секрет (необходим только для TVM)
        auth_token_env: -- переменная окружения, где лежит YQL token (необходим только при выключенном TVM). Не подходит для production!

nodes_storage:
    cache_size:  -- Размер кэша
    ttl: -- TTL
    warm_up:
        nodes_idx: -- ноды, которые надо регулярно подтягивать в кэш
        sleep_time: -- как часто это делать

logging:
    kikimr_level: -- уровень логгирования для базы
    common_level: -- уровень логгирования приложения

dssm:
    path: -- путь до модели
    input_name: -- название входа, на который подавать фразу
    output_name: -- название выхода, откуда брать вектор (эмбеддинг)
    empty_inputs: -- названия входов, на которые надо подать пустые строки (DSSM требует, чтобы на каждый вход было что-то подано)
    cache_size: -- размер кэша (применение модели кэширутеся с помощью LFU)

model:
    thresh: -- порог, по которому отсекаются соседи
    dist_thresh_rel: -- оператор сравнения расстояния и порога. Позволяет задать, хорошее расстояние -- это больше или меньше порога.
    metric: minkowski
    n_neighbors: -- количество соседей, возвращаемых моделью по-умолчанию (если для конкретной ноды не будет указано другое число)
    p: -- параметр расстояния minkowski (при p=2 получим Евклидово расстояние)

server:
    host: -- Хост
    port: -- Порт
    workers: -- Количество процессов для обработки запросов (обычно стоит выставлять в 2 * cores + 1)
    timeout: -- Таймаут

redis:
    turned_on: -- использовать ли Redis
    address: -- адрес для подключения
    maxsize: -- размер пула подключений
```


## Существующие ручки и примеры запросов ##

### /add_node | POST ###
Добавляет новую ноду. Если нода с указанным ID уже существует, то она будет перезаписана.

```json
{
    "threshold": 0.6,
    "node_id": "test_bank_node", 
    "intents": [
        {
            "intent_id": "home_screen",
            "utterances": [
                "домой",
                "в начало",
                "продолжить"
            ]
        },
        {
            "intent_id": "insurance",
            "utterances": [
                "страховка",
                "страхование",
                "у вас есть страховка",
                "можно застраховать"
            ]
        }
    ]
}
```

Тестовый запрос:
```bash
curl -XPOST -H "Content-type: application/json" -d '{"threshold": 0.6, "intents": [{"intent_id": "home_screen", "utterances": ["домой", "в начало", "продолжить"]}, {"intent_id": "insurance", "utterances": ["страховка", "страхование", "у вас есть страховка", "можно застраховать"]}], "node_id": "test_bank_node"}' '127.0.0.1:12345/add_node'
```

Ответ:
```json
{"node_id": "test_bank_node"}
```

### /rem_node | POST ###
Удаляет ноду. Возвращает 200 не зависимо от того, существует ли нода.

```json
{
    "node_id": "test_bank_node"
}
```

Тестовый запрос:
```bash
curl -XPOST -H "Content-type: application/json" -d '{"node_id": "test_bank_node"}' '127.0.0.1:12345/rem_node'
```

Ответ:
```json
{"node_id": "test_bank_node"}
```

### /get_intents | POST ###
Получить интенты для данной фразы и ноды. Если ноды не существует, вернет 400. Возвращает список интентов и списек расстояний до них. Списки отсортированы в порядке возрастания расстояния.

```json
{
    "node_id": "test_bank_node",
    "utterance": "осаго"
}
```

Тестовый запрос:
```bash
curl -XPOST -H "Content-type: application/json" -d '{"node_id": "test_bank_node", "utterance": "осаго"}' '127.0.0.1:12345/get_intents'
```

Ответ:
```bash
{"distances": [0.41638946533203125], "intents": ["insurance"]}
```

### /solomon | GET ###
Ручка, в которую ходит Solomon за данными для мониторингов.

### /ping | GET ###
Ручка, в которую ходит балансер для проверки, жив ли хост.

### /add_graph | POST ###
Добавить граф. Если граф уже существует, то будет перезаписан. Пример запроса можно найти в интеграционных тестах.

### /get_graph_intent | POST ###
Ручка-хэндлер для графового скила. Текущее состояние хранится в стейте навыка (должен быть включен).
Внимание! Ручка неустойчива к изменению графа. Если во время общения пользователя с навыком граф будет изменен, могут полететь ответы 400. При этом они должны пройти после перезапуска навыка пользователем.

### /rem_graph | POST ###
Удалить граф. Возвращает 200 не зависимо от того, существует ли он.

```json
{
    "graph_id": "test_graph"
}
```

Тестовый запрос:
```bash
curl -XPOST -H "Content-type: application/json" -d '{"graph_id": "test_graph"}' '127.0.0.1:12345/rem_graph'
```

Ответ:
```json
{"graph_id": "test_graph"}
```


## Полезные ссылки ##
- [Основной тикет](https://st.yandex-team.ru/PASKILLS-4438)
- [Grafana](https://grafana.yandex-team.ru/d/chyAGA_Wz/penguinarium)
- [Solomon production](https://solomon.yandex-team.ru/admin/projects/dialogovo/shards/dialogovo_pengd_prod_penguinary)
- [Solomon testing](https://solomon.yandex-team.ru/admin/projects/dialogovo/shards/dialogovo_peng_testing_penguinary)
- [YDB production](https://ydb.yandex-team.ru/db/ydb-ru/alice/prod/paskills_penguinary/browser)
- [YDB testing](https://ydb.yandex-team.ru/db/ydb-ru-prestable/alice/development/paskills_penguinary/browser)
- [Nanny production](https://nanny.yandex-team.ru/ui/#/services/catalog/penguinary_production/)
- [Nanny testing](https://nanny.yandex-team.ru/ui/#/services/catalog/dud-testing/)
- [YAV production](https://yav.yandex-team.ru/secret/sec-01e2fyjwjkkb6mvzxtx2s7bhe0/explore/versions)
- [YAV testing](https://yav.yandex-team.ru/secret/sec-01e1bxm8qw6n0hn5kw4fnmc1f8/explore/versions)
- [Logs production](https://yt.yandex-team.ru/hahn/navigation?path=//logs/penguinarium-prod-server-log&)
- [Logs tesing](https://yt.yandex-team.ru/hahn/navigation?path=//logs/penguinarium-test-server-log&)
- [Logbroker produciton](https://lb.yandex-team.ru/logbroker/accounts/paskills/prod/penguinarium?page=browser&type=directory)
- [Logbroker testing](https://lb.yandex-team.ru/logbroker/accounts/paskills/test/penguinarium?page=browser&type=directory)
- [Hitman production](https://hitman.yandex-team.ru/projects/paskils-external-errors/stable-update-dssm-vectors)
- [Hitman testing](https://hitman.yandex-team.ru/projects/paskils-external-errors/testing-update-dssm-vectors)