# Force Discovery
## Переменные окружения:

```MIGRATION_CONFIG_PATH``` - путь к конфигу force-discovery
```YDB_TOKEN``` - токен к базе данных, используется только при выборе ```datasource_type = "db"```

## Формат конфига - YAML

```yaml
---
rps: int
steelix_url: string
steelix_tvm_id: int
skill_id: string
steelix_auth_type: string
oauth_config:
  token: string
tvm_config:
    client_name: string
    port: int
    token: string
device_type: string
datasource_type: string
datasource_parameters: object
```

### Описание полей:
- ```rps``` : значение рпс, которое необходимо поддерживать во время работы скрипта
- ```steelix_url``` : HTTP URL Steelix - хосты смотреть [здесь](https://a.yandex-team.ru/arc/trunk/arcadia/alice/iot/steelix/doc/SH_callback.md#хосты)
- ```steelix_tvm_id```: TVM ID Steelix - смотреть [здесь](https://a.yandex-team.ru/arc/trunk/arcadia/alice/iot/steelix/doc/SH_callback.md#способы-авторизации-и-аутентификации)
- ```skill_id``` : для какого провайдера надо сделать discovery
- ```steelix_auth_type``` : тип авторизации. Поддерживаются 2 типа авторизации - ```oauth``` и ```tvm```.
- ```oauth_config``` : конфиг OAuth авторизации при ```auth_type = oauth```
    - ```token``` : OAuth токен
- ```tvm_config``` : конфиг TVM клиента при  ```auth_type = tvm``
    - ```client_name``` : клиент, от чьего имени надо делать запросы в Steelix
    - ```port``` : порт, на котором запущен tvmtool
    - ```token``` : TVM токен
- ```device_type``` : [тип девайса](https://yandex.ru/dev/dialogs/alice/doc/smart-home/concepts/device-types-docpage/) - по нему можно фильтровать устройства, которые надо обновить. Если фильтрация не нужна, не указывайте это поле в конфиге.
- ```datasource_type``` : источник данных о юзерах, для которых необходимо сделать discovery. Поддерживаются два типа источника данных - ```config``` и ```db```
- ```datasource_parameters``` : два разных формата параметров в зависимости от выбранного ```datasource_type```.
    - ```datasource_type = "db"``` - пользователи будут считываться из таблички ```ExternalUsers``` по адресу базы. Параметры:
    ```yaml
    ---
    prefix: string    // prefix YDB
    endpoint: string  // endpoint YDB
    ```
    - ```datasource_type = "config"``` - пользователи будут считываться из плоского списка прямо из конфига. Параметры:
    ```yaml
    ---
    users_id: [string] // список external_id юзеров (для Tuya Provider-а - bulbasaur-овский user_id, только строкой)
    ```
### Пример конфига
Пример конфига лежит по адресу ```arcadia/alice/iot/steelix/force-discovery/config.yaml```.
