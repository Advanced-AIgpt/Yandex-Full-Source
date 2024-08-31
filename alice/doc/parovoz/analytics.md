# Аналитика в Паровозе

Для аналитики можно использовать параметры:


## ParentRequestId

[ParentRequestId](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/analytics/analytics_info.proto?rev=r7758492#L26). Идентификатор первого запроса (на который случился newSession). С его помощью можно отфильтровать все запросы в рамках одной паровозной сесии и найти запрос, с которого началась сессия. Запросы внутри одной сессии имеют одинаковый `ParentRequestId`. Если ParentRequestId = [RequestId](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/speechkit/request.proto?rev=r7758492#L27) - значит это запрос, который активировал сценарий.
Поле можно найти в вандерлогах в колонке `speechkit_response`, в поле `AnalyticsInfo`.

```json
(
    "header": (
        "parent_request_id": "01cbb70c-a945-4287-b6e2-f6f2ff0b3524",
        "ref_message_id": "b4ee8948-2d7e-4d03-8b87-972977385d7d",
        "request_id": "0f062403-5862-4286-be2f-bde7f39e37eb",
        "response_id": "e078e32c-99698ed8-87f8e43b-caed1eb4",
        "sequence_number": 70,
        "session_id": "99e4430b-cd60-470e-aa65-018468d33e06"
    ),
    "megamind_analytics_info": (
        "analytics_info": {
            "HollywoodMusic": (
                "ParentProductScenarioName": "music",
                "ParentRequestId": "01cbb70c-a945-4287-b6e2-f6f2ff0b3524",
                "frame_actions": {},
                ...
            ),
            ...
        },
        ...
    ),
...
)
```

## ParentProductScenarioName

[ParentProductScenarioName](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/analytics/analytics_info.proto?rev=r7758492#L27). С помощью него можно узнать кто инициировал запуск данного сценария (т.е. родительский сценарий). Поле можно найти в вандерлогах в колонке `speechkit_response`, в поле `AnalyticsInfo`. 


```json
(
    "header": (
        "parent_request_id": "01cbb70c-a945-4287-b6e2-f6f2ff0b3524",
        "ref_message_id": "b4ee8948-2d7e-4d03-8b87-972977385d7d",
        "request_id": "0f062403-5862-4286-be2f-bde7f39e37eb",
        "response_id": "e078e32c-99698ed8-87f8e43b-caed1eb4",
        "sequence_number": 70,
        "session_id": "99e4430b-cd60-470e-aa65-018468d33e06"
    ),
    "megamind_analytics_info": (
        "analytics_info": {
            "HollywoodMusic": (
                "ParentProductScenarioName": "music",
                "ParentRequestId": "01cbb70c-a945-4287-b6e2-f6f2ff0b3524",
                "frame_actions": {},
                ...
            ),
            ...
        },
        ...
    ),
...
)
```

## AnalyticsTrackingModule

Структура [AnalyticsTrackingModule](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/atm.proto?rev=r8267181#L11). 
В ней хранится информация:
 - источник формирования запроса ([Origin](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/atm.proto?rev=r8267181#L28));
 - предназначение запроса ([Purpose](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/atm.proto?rev=r8267181#L33)) (для чего был послан этот запрос, что ожидается в ответе);
 - опциональное поле с произвольной информацией об источнике запроса [OriginInfo](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/atm.proto?rev=r7940302#L36).
 Структуру можно найти в вандерлогах в колонке `speechkit_request`, в поле `Analytics`.

 ```json
{
    "origin": "Marketing",
    "purpose": "show_weather",
    "origin_info": "<ID рекламной компании>"
}
```

## Какие данные можно получить

1. Подсчет timespent (tlt+tts — время видео/музыки + время речи) сценария внутри паровозной сессии.
Подсчитывается с помощью ParentProductScenarioName.

2. Определение активационного запроса.
С помощью `ParentRequestId` можно отфильтровать все запросы в рамках одной паровозной сесии и найти запрос, с которого началась сессия. Запросы внутри одной сессии имеют одинаковый `ParentRequestId`. Если `ParentRequestId` = `RequestId` - значит это запрос, который активировал сценарий.

3. Подсчет timespent или tlt (time to listen) всего паровозного сценария.
По равенству `ParentRequestId` = `RequestId` можно найти "голову паровоза". Далее, пока `ParentRequestId` совпадает в вандерлогах в колонке `speechkit_response`, в поле `AnalyticsInfo`, необходимо продолжать атрибуцировать timespent/tlt к этой же сценарной сессии/сессии прослушивания. Как только один из параметров поменяется, сессия завершится и можно подсчитать метрику timespent/tlt.




