# Внешнее взаимодействие со сценарием
Часто возникает потребность иницировать ответ Алисы без прямого запроса пользователя. Например, например нажатие кнопки на пульте, переходо по [диплинкe](https://wiki.yandex-team.ru/alice/megamind/protocolscenarios/proto/deeplinks/), событие "конец фильма", а также межсценарное взаимодействие.
## Запрос с Semantic Frame
Данный механизм позволяет отправить запрос с семантический фреймом. В процессе обработки запроса ответ Бегемота заменяется на фрейм, который пришел в самом запросе.

### Формат запроса
Для запроса необходимо сформировать `server_action` с именем `@@mm_semantic_frame` и передать необходимые значения в поле `payload`.

Пример запроса с семантическим фреймом.
```json
"event": {
    "name": "@@mm_semantic_frame",
    "payload": {
        "typed_semantic_frame": {
            "search_semantic_frame": {
                "query": {
                    "string_value": "что такое ананас"
                }
            }
        },
        "analytics": {
            "origin": "Scenario",
            "purpose": "get_factoid"
        }
    },
    "type": "server_action"
}
```

{% note info %}

Используйте названия из параметров полей `json_name` в протоколе для формирования json содержимого в запросе. Например, поле `search_semantic_frame` соотвествует `json_name` для [этого поля](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/frame.proto?rev=7142544#L122).

{% endnote %}

`Payload` состоит из 2 **обязательных** полей:
- `typed_semantic_frame` - содержимое типизированного семантик фрейма, которое соотвествует описанию в [протоколе](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/frame.proto?rev=7142544#L118);
- `analytics` - содержимое модуля отслеживания аналитики, которое соотвествует описанию в [протоколе](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/atm.proto), состоит из следующих полей:
    - `origin` - источник формирования запроса, соответствует описанию enum'a в протоколе;
    - `purpose` - намерение пользователя при совершении действия (для чего был послан этот запрос, что ожидается в ответе);
    - `origin_info` - опциональное поле с произвольной информацией об источнике запроса;
    - `product_scenario` - имя продуктового сценария, согласованное с аналитиками. Одни и те же сценарии megamind могут вызываться из различных продуктовых сценариев, а так же несколько сценариев megamind могут быть склеены одним продуктовым сценарием.

### Примеры блока аналитики
1. Пользователь переходит в ПП по ссылке с веб-сайта навыков Алисы.
    ```json
    {
        "origin": "Web",
        "purpose": "skill_activation"
    }
    ```
1. Пользователь нажимает на пуш с погодой.
    ```json
    {
        "origin": "Marketing",
        "purpose": "show_weather",
        "origin_info": "<ID рекламной компании>"
    }
    ```
1. На колонку прилетает пуш "player_next_track".
    ```json
    {
        "origin": "SmartSpeaker",
        "purpose": "get_next_track"
    }
    ```

### Работа с фреймом
[TypedSemanticFrame](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/frame.proto?rev=7142544#L118) при запросе в сценарий трансформируется в [SemanticFrame](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/frame.proto?rev=7142544#L126) (подробнее о разборе фреймов читайте в разделе [Фреймы и формы](../frames.md#frames-forms)).

Связывание фреймов происходит по значению опции `SemanticFrameName` в конкретного фрейма в протоколе. Например, для типизированного [поискового фрейма](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/frame.proto?rev=7142544#L122) связавание происходит с фреймом [personal_assistant.scenarios.search](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/frame.proto?rev=7142544#L113). Чтобы в сценарий начал приходить этот фрейм достаточно прописать его (`personal_assistant.scenarios.search`) в [AcceptedFrames в конфиге сценария](config.md#format).

### Поддержка нового фрейма
1. Добавьте новый фрейм в oneof [TypedSemanticFrame](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/frame.proto?rev=7142544#L121). Рассмотрим пример с [TSearchSemanticFrame](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/frame.proto?rev=7142544#L111).
    ```protobuf
    message TSearchSemanticFrame {
        // Данный параметр связывает TSemanticFrame и TTypedSemanticFrame
        option (SemanticFrameName) = "personal_assistant.scenarios.search";

        // Значение SlotName используется при трансформации TTypedSemanticFrame в TSemanticFrame
        TStringSlot Query = 1 [json_name = "query", (SlotName) = "query"];
    }

    message TTypedSemanticFrame {
        oneof Type {
            TSearchSemanticFrame SearchSemanticFrame = 1 [json_name = "search_semantic_frame"];
        }
    }
    ```
1. Добавьте тест на трансформацию фрейма в [walker_ut.cpp](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/library/walker/walker_ut.cpp?rev=7211254#L1231).
1. Добавьте [интеграцинный тест](https://a.yandex-team.ru/arc/trunk/arcadia/alice/tests/integration_tests) на запрос с семантическим фреймом.
