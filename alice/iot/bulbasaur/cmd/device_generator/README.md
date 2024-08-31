## Запуск через Sandbox

Запустить Sandbox задачу типа IOT_DEVICE_GENERATOR c нужными параметрами.
Подробная инструкция находится по 
[этой ссылке](https://wiki.yandex-team.ru/alicetesting/Umnyjj-dom/#sozdanievirtualnyxustrojjstv).


##  Запуск через командную строку
При первом запуске device_generator нужно будет указать user_id и login, чтобы создался новый пользовательский конфиг.


Для запуска device_generator необходимы 3 вещи:

1. Корректные user_id, login в user_config.yaml или переданные user_id, login в аргументах командной строки к бинарнику.
user_config.yaml будет создан после первого запуска.
Если одновременно user_id в user_config.yaml не ноль и есть параметр -user_id=..., то берется значение из параметра.
Аналогично, если login в user_config.yaml непустое и есть параметр -login=..., то берется значение из параметра.

2. Правильно заданные пути к конфигам устройств после всех флагов.
Если заданы неименованные аргументы командной строки, то из них берутся конфиги, иначе во время выполенения происходит ошибка отсутствия
конфигов устройств.
В ./device_configs/ лежат корректные конфиги для некоторых устройств.

3. Валидный доступ к базе данных. Если параметр -db=... не указан, то используется beta БД,
указав -db=./database_configs/db_prod.yaml, получаем доступ к prod БД.
Если нужен кастомный доступ к БД, то его можно указать в файле db_custom.yaml
и вызвать device_generator с параметром -db=./database_configs/db_custom.yaml или можно указать путь к произвольному
YAML конфигу БД.
Если поле ydb_token пустое, то токен берется из переменной окружения YDB_TOKEN, иначе берется из поля ydb_token.


Примеры:

1. ```
    ./device_generator -user_id=326856782 -login=bluemango -db=./database_configs/db_beta.yaml ./device_configs/coffee_maker.yaml
   ```

2. ```
   ./device_generator -user_id=326856782 ./device_configs/coffee_maker.yaml
   ```
    В файле user_config.yaml:
    ```(yaml)
    user_id: 326856782
    login: bluemango
    ydb_token: ""
    ```
Если device_config.yaml одинаковы, то примеры 1 и 2 эквивалентны: добавляют устройство из ./device_configs/coffee_maker.yaml
в beta БД для одного и того же пользователя.

3. ```
   ./device_generator ./device_configs/*
   ```
    Добавляет все конфиги из ./device_configs
4.  ```
    ./device_generator ./device_configs/coffee_maker.yaml ./device_configs/vacuum_cleaner.yaml
    ```
    Добавить кофеварку и пылесос.

