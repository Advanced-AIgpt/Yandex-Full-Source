# Описание

Tuya Adapter для [Smart Home API](https://yandex.ru/dev/dialogs/smart-home/)

# Платформа:

Бекенд в RTC: [nanny](https://nanny.yandex-team.ru/ui/#/services/) -> quasar -> tuya_adapter <br>
БД: <br>
    - [ydb:production/iotdb](https://ydb.yandex-team.ru/db/ydb-ru/quasar/production/tuya_adapter/browser)<br>
    - [ydb:prestable/iotdb](https://ydb.yandex-team.ru/db/ydb-ru-prestable/quasar/prestable/tuya_adapter/browser)<br>
    - [ydb:development/iotdb](https://ydb.yandex-team.ru/db/ydb-ru-prestable/quasar/development/tuya_adapter/browser)<br>
Сборка образов: [QUASAR_IOT_TUYA_ADAPTER_DOCKER_BUILD](https://testenv.yandex-team.ru/?screen=job_history&database=alice-iot&job_name=QUASAR_IOT_TUYA_ADAPTER_DOCKER_BUILD)

## Сборка локально

### Пререквизиты

При сборке через `ya make` используются не встроенные в Go либы, а контрибы с Аркадии, так что их надо скачать заранее:

```(sh)
cd ~/arcadia
ya make --checkout -j0 contrib/go
```

### Просто собрать бинарь с сервером

```(sh)
cd ~/arcadia/alice/iot/adapters/tuya_adapter
ya make
```

Бинарь `server` появится в `~/arcadia/alice/iot/adapters/tuya_adapter/cmd/server/`

### Собрать Docker образ

Сборка осуществляется с помощью `ya package`, который на самом деле сперва запускает `ya make`, а затем создает окружение для сборки контейнера, используя описание из `pkg.json`.

```
cd ~/arcadia/alice/iot/adapters/tuya_adapter
ya package pkg.json --docker --docker-repository=iot --target-platform=DEFAULT-LINUX-X86_64
```

Будет собран образ `registry.yandex.net/quasar/tuya_adapter:{revision}`, где `{revision}` - текущая ревизия локального кода

Если нужно сразу запушить, то надо добавить параметр `--docker-push`:

```
cd ~/arcadia/alice/iot/adapters/tuya_adapter
ya package pkg.json --docker --docker-repository=iot --docker-push --target-platform=DEFAULT-LINUX-X86_64
```

## Копия БД в YT
Данные хранятся [в YT](https://yt.yandex-team.ru/hahn/navigation?path=//home/iot/backup/tuya-ydb/); копируются
из продакшен-базы каждую ночь ([sandbox-scheduler](https://sandbox.yandex-team.ru/scheduler/21272)). Отлично подходят
для аналитических запросов, в т.ч. регулярных.

Чтобы развернуть разработческую базу из YT-копии нужно запустить такой запрос, предварительно заменив в нем ```$ydb_database```:
[YT_TO_YDB_RESTORE](https://yql.yandex-team.ru/Operations/XpBKNQtcP68VCKQLmrdbx-JkRpJ-6-neTKbiOYewQao=)

# Документация на Tuya Adapter Smart Home API

Адреса апи:

* https://quasar.yandex.ru/tuya -- для внешней сети
* https://quasar-iot-tuya-adapter.yandex.ru -- для внутренней


## Описание ручек


### Pairing

#### POST /m/tokens

Получить `pairing_token` и `cipher` для пейринга устройства.

##### Пример запроса

```json
{
    "s": "ssid",
    "p": "12345678",
    "t": 1
}
```

##### Описание полей

|Поле|Тип|Описание|
| ------------- |:-------------:| :-----|
|s|String|SSID wifi сети|
|p|String|Пользовательский пароль от wifi|
|t|Integer|Тип подключения: EZ=0, AP=1|

##### Пример ответа

```json
{
	"region": "EU",
	"token": "8dz2VaFk",
	"secret": "b8Co",
	"cipher": "aiblcLSNq5UGyL4Xf5wpIw==",
	"status": "ok"
}
```

##### Описание полей

|Поле|Тип|Описание|
| ------------- |:-------------:| :-----|
|status|String|Статус обработки запроса - `ok/error`|
|region|String|Значение региона|
|token|String|Значение токена|
|secret|String|Значение секрета|
|cipher|String|Зашифрованный wifi пароль|
|msg|String|Описание ошибки, если `status=error` (необязательное поле)|

Для успешного пейринга нужно сконкатенировать строки `region` + `token` + `secret` для получения `pairing_token`<br>
`pairing_token` в свою очередь передается в метод SDK

#### POST /q/v1.0/tokens

Получить `pairing_token` и `cipher` для пейринга устройства. Формат тела запроса и формат ответа идентичны таковым для ручки ```POST /m/tokens```. 
Единственное отличие - метод авторизации в данной ручке ```OAuth```.
Ручка сделана специально для голосового подключения устройств. 

#### GET /v1.0/tokens/{token}/devices

Получить устройства, запейренные с помощью `pairing_token`<br>
**Внимание!** Эта ручка принимает значение `token` из метода `GET /iot/v1.0/token`, не путать с `pairing_token`

##### Пример ответа

```json
{
    "status": "ok",
    "successDevices": [
        {
            "id": "41803548bcddc2e78d5c",
            "name": "Лампа",
            "productId": "jQRc7Cgy8OVzwSRG",
            "ip": "195.208.27.160",
            "uuid": "41803548bcddc2e78d5c"
        }
    ],
    "errorDevices": []
}
```

##### Описание полей

|Поле|Тип|Описание|
| ------------- |:-------------:| :-----|
|status|String|Статус обработки запроса - `ok/error`|
|successDevices|List<Object>|Список успешно запейренных устройств|
|errorDevices|List<Object>|Список неуспешно запейренных устройств|
|msg|String|Описание ошибки, если `status=error` (необязательное поле)|


### Devices

#### GET /iot/v1.0/devices

Получить полный список умных устройств пользователя вместе с их статусом

##### Пример ответа

```json
{
	"devices": [{
		"config": {
			"mode": "white",
			"switch": false,
			"white": {
				"brightness": 100
			},
			"colour": {
				"h": 1,
				"s": 100,
				"v": 100
			}
		},
		"name": "A70RGBW",
		"platform": "led",
		"id": "41803dc2e78c63",
		"location": "unknown"
	}],
	"status": "ok"
}
```

##### Описание полей

|Поле|Тип|Описание|
| ------------- |:-------------:| :-----|
|status|String|Статус обработки запроса - `ok/error`|
|devices|List<Object>|Список с устройствами пользователями и их настройками|
|msg|String|Описание ошибки, если `status=error` (необязательное поле)|

#### GET /iot/v1.0/device/{device_id}/state

Получить информацию о текущем состоянии устройства с айди `device_id`

##### Пример ответа

```json
{
	"device": {
		"config": {
			"mode": "white",
			"switch": false,
			"white": {
				"brightness": 100
			},
			"colour": {
				"h": 1,
				"s": 100,
				"v": 100
			}
		},
		"name": "A70RGBW",
		"platform": "led",
		"id": "41803dc2e78c63",
		"location": "unknown"
	},
	"status": "ok"
}
```

##### Описание полей

Поля зависят от типа устройства. Ниже пример для лампочки:

|Поле|Тип|Описание|
| ------------- |:-------------:| :-----|
|status|String|Статус обработки запроса - `ok/error`|
|device|Object|Объект с описанием устройства|
|config|Object|Специфичные для устройства настройки|
|name|String|Имя устройства|
|platform|String|Тип устройства (например, `led` для лампочки)|
|id|String|Уникальный id устройства|
|location|String|Локация устройства (комната, кухня и тп)|
|msg|String|Описание ошибки, если `status=error` (необязательное поле)|

Описание настроек для лампочки (поле `config`):

|Поле|Тип|Диапазон значений|Описание|
| ------- |:------------:|:------:| :-----|
|mode|String|`[white, colour]`|Текущий режим работы|
|switch|Bool|-|Статус устройства (вкл/выкл)|
|white|Object|-|Настройки для режима работы `white`|
|white.brightness|Integer|1-100|Яркость в режиме работы `white`|
|colour|Object|-|Настройки для режима работы `colour`|
|colour.h|Integer|1-360|hue|
|colour.s|Integer|0-100|saturation|
|colour.v|Integer|1-100|value (яркость)|

#### POST /iot/v1.0/device/{device_id}/commands

Отправить команды устройству. Команды передаются в теле POST-запроса в формате JSON.

##### Примеры запросов

Белая лампа

```json
{
	"switch": true,
	"platform": "led",
	"mode": "white",
	"options": {
	    "brightness": 75
	}
}
```

Цветная лампа:

```json
{
	"switch": true,
	"platform": "led",
	"mode": "colour",
	"options": {
	    "h": 255,
	    "s": 45,
	    "v": 75
	}
}
```

##### Описание полей

Поля зависят от типа устройства. Ниже пример для лампочки:

|Поле|Тип|Описание|
| ------------- |:-------------:| :-----|
|switch|Bool|Статус: `true` - включено, `false` - выключено|
|platform|String|Тип устройства|
|mode|String|Режим работы устройства|
|options|Object|Дополнительные свойства для указанного режима работы устройства|

 `Options` для лампы:

|Поле|Тип|Диапазон значений|Описание|
| ------- |:------:|:------:| :-----|
|h|Integer|1-360|hue (только для `mode=colour`)|
|s|Integer|0-100|saturation (только для `mode=colour`)|
|v|Integer|1-100|value (яркость) (только для `mode=colour`)|
|brightness|Integer|1-100|Яркость (только для `mode=white`)|

##### Пример ответа

```json
{
	"status": "ok",
	"message": "something"
}
```

##### Описание полей

|Поле|Тип|Описание|
| ------------- |:-------------:| :-----|
|status|String|Статус обработки запроса - `ok/error`|
|message|String|Описание ошибки, если `status=error` (необязательное поле)|

#### DELETE /iot/v1.0/device/{device_id}

Удаляет устройство пользователя.

##### Пример ответа

```json
{
	"status": "ok",
	"message": "something"
}
```

##### Описание полей

|Поле|Тип|Описание|
| ------------- |:-------------:| :-----|
|status|String|Статус обработки запроса - `ok/error`|
|message|String|Описание ошибки, если `status=error` (необязательное поле)|
