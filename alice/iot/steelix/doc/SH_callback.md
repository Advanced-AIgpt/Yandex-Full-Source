# Smart Home Callback API

## Общее

### Хосты

- production: `https://dialogs.yandex.net/api`
- priemka: `https://dialogs.priemka.voicetech.yandex.net/api`
- testing: `https://dialogs.test.voicetech.yandex.net/api`
- testing-mimino: `https://dialogs.test.voicetech.yandex.net/api-mimino`
- load-testing: `https://{имя контейнера}/api`

### Способы авторизации и аутентификации

- OAuth-токен пользователя-владельца навыка для [API Диалогов](https://yandex.ru/dev/dialogs/alice/doc/resource-upload-docpage/#http-images-load__auth)
- сервисный TVM тикет для внутренних партнеров. Передается в заголовке `X-Ya-Service-Ticket`
  - production: `2016427`
  - priemka: `2016207`
  - testing: `2016205`
  - testing-mimino: `2016205`
  - load-testing: `2016205`

## Discovery

### POST /v1/skills/{skillId}/callback/discovery

Тело запроса:
```(json)
{
    "ts": float64, // в секундах - знаки после запятой для разных точностей
    "payload": {
        "user_id": string // внешний user_id пользователя у провайдера
    }
}
``` 
Ручка, через которую провайдеры могут сообщить в [Bulbasaur](https://a.yandex-team.ru/arc/trunk/arcadia/alice/iot/bulbasaur) о необходимости обновить информацию об устройствах пользователя - 
сходить в ручку навыка `GET /v1.0/user/devices`, получить актуальную информацию об известных устройствах и узнать о новых.

## State
 
### POST /v1/skills/{skillId}/callback/state

Тело запроса:
```(json)
{
    "ts": float64, // в секундах - знаки после запятой для разных точностей
    "payload": {
        "user_id": string, // внешний user_id пользователя у провайдера
        "devices": [
            {
                "id": string,
                "capabilities": [
                    {
                        "type": string,
                        "state": {}
                    }
                ] 
                "properties": [
                    {
                        "state": {}
                    }
                ]
            }
        ]
    }
}
``` 
Ручка, через которую провайдеры могут сообщить в [Bulbasaur](https://a.yandex-team.ru/arc/trunk/arcadia/alice/iot/bulbasaur) о измененных состояниях устройств.

## Push Discovery

### POST /v1/skills/{skillId}/callback/push-discovery

Тело запроса:
```(json)
{
    "ts": float64, // в секундах - знаки после запятой для разных точностей
    "payload": {
        "user_id": string, // внешний user_id пользователя у провайдера
        "devices": [device] 
    }
}
``` 
Ручка, через которую провайдеры могут сообщить в [Bulbasaur](https://a.yandex-team.ru/arc/trunk/arcadia/alice/iot/bulbasaur) информацию об устройствах пользователя. 
Ключевое отличие от ручки `Discovery` в том, что провайдеры сразу передают в теле данные об устройствах. Bulbasaur отвечает на запрос синхронно и можно сразу получить результат операции.

Структура поля `payload` идентична таковой в ответе ручки навыка `GET /v1.0/user/devices`. Пример можно посмотреть в [протоколе работы УД](https://yandex.ru/dev/dialogs/alice/doc/smart-home/reference/get-devices-docpage/).

Ответ сервера:
```(json)
{
    "status": string, // ok/error
    "request_id": string,
    "error_code": string, // присутствует в теле, только если status == "error"
    "error_message": string // может отсутствовать вообще
}
```
Возможные коды ошибок:
- `UNKNOWN_USER` - пользователя не существует в базе УД (для внутренних провайдеров пользователь в такой ситуации будет создан)
- `BAD_REQUEST` - запрос пришел с неправильным телом
- коды ошибок для обычного `Discovery` из [протокола работы УД](https://yandex.ru/dev/dialogs/alice/doc/smart-home/reference/get-devices-docpage/).
