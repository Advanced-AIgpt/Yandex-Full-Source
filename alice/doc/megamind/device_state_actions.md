# Голосовые кнопки в DeviceState {#device_state_btns}

Голосовые кнопки в DeviceState — механизм, позволяющий клиенту задавать специфичное поведение на голосовые запросы в разных контекстах.
C помощью данного механизма можно реализовать интерактивный UI с голосовым управлением для девайсов с экраном, где состояние галереи может меняться пользователем без запроса к серверу. Например, команда "продолжи" на экране видео означает продолжение видео, а на экране музыки — продолжение музыки. Больше примеров описано [тут](#examples).

Ключевыми составляющими являются [активационный](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/conditional_action.proto?rev=r8970646#L16) и [эффективный](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/conditional_action.proto?rev=r8970646#L17) фреймы кнопки. Разбор активационного фрейма в запросе имитирует нажатие кнопки и, как результат, запускает обработку эффективного фрейма. Такие кнопки описываются в DeviceState запроса.

## Использование

Клиент формирует в запросе список голосовых кнопок, которые релевантны текущему состоянию девайса. Например:
- открыта видео галерея — формируются кнопки для её перелистывания, открытия фильмов по номеру, открытие видимых пользователю фильмов по названию и т.д.
- открыт музыкальный плеер — формируются кнопки для обработки "дальше", "назад", "лайк" и т.д.
- открылось всплывающее окно с сообщением — формируется кнопка "Окей"/"Понятно" для закрытия окна.

## Передача голосовых кнопок из сценария

Вместе с ответом на запрос, сценарий может вернуть набор голосовых кнопок, которые прилетают на клиент в виде директивы. Клиент сам выбирает нужные кнопки (в зависимости от экрана и т.д.) и присылает их в запросе в DeviceState. Мегамайнд не запоминает кнопки, которые вернул сценарий, а обрабатывает только те, что пришли в DeviceState.

## Формат передачи голосовых кнопок в Megamind

Megamind обрабатывает кнопки, пришедшие с клиента в поле [DeviceState](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/device_state.proto?rev=r8970646#L119) -> [ActiveActions](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/device_state.proto?rev=r8970646#L667) -> [ScreenConditionalActions](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/device_state.proto?rev=r8970646#L664).

`ScreenConditionalActions` описывает голосовые кнопки, доступные на каждом экране.

{% note info "" %}

Множественные экраны не поддержаны в Мегамайнде, поэтому все кнопки нужно передавать в одном списке на одном экране с названием **main**.

{% endnote %}

Голосовая кнопка описывается протобуфом [TConditionalAction](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/conditional_action.proto?rev=r8970646#L13) и содержит в себе пару из активационного фрейма (`ConditionalSemanticFrame`) и эффективного (`EffectFrameRequestData`).

Активационный фрейм является частично или полностью заполненным [TTypedSemanticFrame'ом](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/frame.proto?rev=r9117238#L1386).

Эффективный фрейм является полность заполненным типизированым фреймом с дополнительной аналитической информацией ([TSemanticFrameRequestData](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/frame.proto?rev=r9050756#L1435)). Этот фрейм будет отправлен на обработку в сценарии.

Если с клиента придет несколько голосовых кнопок, которые могут активироваться на голосовой запрос, то использоваться будет только одна кнопка, которая идет раньше в [списке](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/device_state.proto?rev=r8970646#L656). Поэтому более частные кнопки стоит указывать первыми. 

### Пример описания голосовой кнопки в DeviceState:

```json
"device_state":
{
    ...

    "active_actions": {
        "screen_conditional_action": {
            "main": {
                "conditional_actions": [
                    // описание кнопки
                    {
                        "conditional_semantic_frame": {
                            "player_continue_semantic_frame": {
                            }
                        },
                        "effect_frame_request_data": {
                            "analytics": {
                                "origin": 1,
                                "origin_info": "some_origin_info",
                                "product_scenario": "some_psn",
                                "purpose": "some_purpose"
                            },
                            "origin": {},
                            "typed_semantic_frame": {
                                "player_pause_semantic_frame": {}
                            }
                        }
                    },
                    // --------------
                ]
            },
        }
    }
    ...
}

```
Если такая голосовая кнопка придет в запросе к Мегамайнду, а пользователь скажет фразу, которая матчится на фрейм `player_continue_semantic_frame` (например `"продолжи"`), то Мегамайнд будет обрабатывать эффективные фрейм `player_pause_semantic_frame`.

Если несколько `ConditionalSemanticFrame` матчатся на запрос пользователя, то будет обработана кнопка, которая находится раньше в списке.

### Запрос с условием

При сравнении фрейма, который разобрался на пользовательский запрос, и `ConditionalSemanticFrame`, также учитываются и слоты. Если в `ConditionalSemanticFrame` указан слот фрейма, то его значение должно совпадать со значением слота в разобранном фрейме. Если же слот в `ConditionalSemanticFrame` не указан, то он будет проигнорирован при сравнении. Пример:

```json
"conditional_actions": [
    // голосовая кнопка с conditional_semantic_frame с заполненным слотом player
    {
        "conditional_semantic_frame": {
            "player_continue_semantic_frame": {
                "player": "radio"
            }
        },
        "effect_frame_request_data": {
            "analytics": {
                "origin": 1,
                "origin_info": "some_origin_info",
                "product_scenario": "some_psn",
                "purpose": "some_purpose"
            },
            "origin": {},
            "typed_semantic_frame": {
                "player_pause_semantic_frame": {}
            }
        }
    },
    // голосовая кнопка без заполненных слотов
    {
        "conditional_semantic_frame": {
            "player_continue_semantic_frame": {
            }
        },
        "effect_frame_request_data": {
            "analytics": {
                "origin": 1,
                "origin_info": "some_origin_info",
                "product_scenario": "some_psn",
                "purpose": "some_purpose"
            },
            "origin": {},
            "typed_semantic_frame": {
                "player_shuffle_semantic_frame": {}
            }
        }
    },
```

Для кнопок выше возможны следующие ситуации:
1) Пользователь сказал `"продолжи радио"`. Разобрался фрейм `player_continue_semantic_frame` со слотом `player="radio"`. Такой фрейм матчится на `conditional_semantic_frame` первой кнопки и Мегамайнд обраотает эффективный фрейм этой кнопки – `player_pause_semantic_frame`
2) Пользователь сказал `"продолжи музыку"`. Разобрался фрейм `player_continue_semantic_frame` со слотом `player="music"`. Первая кнопка не подойдет, так как значения в слоте `player` не совпадают. Для второй кнопки в `conditional_semantic_frame` не указан слот `player` и он игнорируется при сравнении. Соответственно вторая кнопка подходит и Мегамайнд обрабатывает эффективный фрейм `player_shuffle_semantic_frame`
3) Пользователь сказал `"продолжи"` (см п2)
4) Пользователь сказал `"домой"`, Мегамайнд не найдет соответствующую голосовую кнопку и запустит стандартный пайплайн.

## Как вернуть голосовые кнопки из сценария и обработка на клиенте
Для того, чтобы вернуть голосовые кнопки из сценария, требуется в ответе сценария заполнить поле [ConditionalActions](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/scenarios/response.proto?rev=r9030544#L325). Это поле является соответствие некоторого `ConditionalActionId` в кнопку [TConditionalAction](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/conditional_action.proto?rev=r8970646#L13). `ConditionalActionId` используется клиентом, чтобы провести соответствие между кнопкой и любым другим элементом. Подразумевается, что сценарий пометит элемент (наприме дивную карточку) этим `ConditionalActionId`.
Мегамаинд в свою очередь вернет директиву со списком всех добавленных кнопок и их `ConditionalActionId`. Пример директивы:

```json
{
    "name": "add_conditional_actions",
    "sub_name": "add_conditional_actions",
    "payload": {
        "action_1": { // ConditionalActionId
            "effect_frame_request_data": {
                "typed_semantic_frame": {
                    "search_semantic_frame": {
                        "query": {
                            "string_value": "ok"
                        }
                    }
                },
                "origin": {...},
                "analytics": {...}
            },
            "conditional_semantic_frame": {
                "search_semantic_frame": {
                    "query": {
                        "string_value": "how are you?"
                    }
                }
            }
        },
        "action_2": {
            "effect_frame_request_data": {
                "typed_semantic_frame": {
                    "search_semantic_frame": {
                        "query": {
                            "string_value": "net"
                        }
                    }
                },
                "origin": {...},
                "analytics": {...}
            },
            "conditional_semantic_frame": {
                "player_pause_semantic_frame": {}
            }
        }
    },
    "type": "client_action"
}
```

## Еще примеры голосовых кнопок: {#examples}

### На фразы вида "дальше" включается конкретный музыкальный трек

```json
"device_state":
{
    "active_actions": {
        "screen_conditional_action": {
            "main": {
                "conditional_actions": [
                    {
                        "conditional_semantic_frame": {
                            "player_next_track_semantic_frame": {
                            }
                        },
                        "effect_frame_request_data": {
                            "analytics": {
                                "origin": 1,
                                "origin_info": "some_origin_info",
                                "product_scenario": "some_psn",
                                "purpose": "some_purpose"
                            },
                            "origin": {},
                            "typed_semantic_frame": {
                                "music_play_semantic_frame": {
                                    "object_type": {
                                        "enum_value": "Track"
                                    },
                                    "object_id": {
                                        "string_value": "track_id"
                                    }
                                }
                            }
                        }
                    },
                ]
            },
        }
    }
    ...
}

```

### Пример item selection:

{% note info "" %}

В данном примере используется `alice.mordovia_video_selection` фрейм, у которого пока нет типизированного описания.
Эффективные фреймы `video_play_semantic_frame` не являются хорошим примером, но сейчас нет фрейма по включению определенного фильма/видео например по kinopoisk_id.

{% endnote %}

```json
{
    "active_actions": {
        "screen_conditional_action": {
            "main": {
                "conditional_actions": [
                    {
                        "conditional_semantic_frame": {
                            "alice.mordovia_video_selection": {
                                "video_index": {
                                    "num_value": 1
                                }
                            }
                        },
                        "effect_frame_request_data": {
                            "analytics": {...},
                            "origin": {...},
                            "typed_semantic_frame": {
                                "video_play_semantic_frame": {
                                    "search_text": "гарри поттер и философский камень"
                                }
                            }
                        }
                    },
                    {
                        "conditional_semantic_frame": {
                            "alice.mordovia_video_selection": {
                                "video_index": {
                                    "num_value": 2
                                }
                            }
                        },
                        "effect_frame_request_data": {
                            "analytics": {...},
                            "origin": {...},
                            "typed_semantic_frame": {
                                "video_play_semantic_frame": {
                                    "search_text": "гарри поттер и тайная комната"
                                }
                            }
                        }
                    }
                ]
            }
        }
    }
}
```


## Клиентские сущности в DeviceState

Вместе с [кнопками](#device_state_btns), есть возможность передать в запросе значения [сущностей](https://docs.yandex-team.ru/alice-scenarios/nlu/), которые будут использоваться бегемотом при разборе запроса.
Например, для реализации айтемселекции по названию, можно передать сущность, которая содержит базовые синонимы для названий всех айтемов и используется в [гранете](https://docs.yandex-team.ru/alice-scenarios/nlu/granet/) для фрейма, который будет передан в кнопках как [ConditionalSemanticFrame](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/conditional_action.proto?rev=r8970646#L16).

### Формат передачи сущностей в Megamind

Сущности приходят в megamind  в запросе от клиента в поле [DeviceState](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/device_state.proto?rev=r9168679#L119) -> [ExternalEntitiesDescription](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/device_state.proto?rev=r9182611#L689) списком.

Описание сущности:

```json
{
    "external_entities_description": [
        // первая сущность
        {
            // название. сущность с этим именем будет разбираться бегемотом по заданным синонимам
            "name": "nonsense_entity_name"
            // элементы сущности
            "items": [
                // первый элемент сущности
                {
                    // список синонимов для элемента
                    "phrases": [
                        {
                            "phrase": "war"
                        },
                        {
                            "phrase": "battle"
                        }
                     ],
                     // значение, которое заполнится в слоте при матче сущности
                     "value": {
                         "string_value": "first_element"
                     }
                },
                // второй элемент сущности
                {
                    "phrases": [
                        {
                            "phrase": "второй"
                        }
                     ],
                     "value": {
                         "string_value": "second_element"
                     }
                },
                // третий элемент сущности
                { ... }
            ]
        },
        // вторая сущность
        { ... }
    ]
}
```

{% note info "" %}

Введено ограничение на количество синонимов для одного элемента. Указать можно максимум 100 фраз. Если список синонимов длиннее, то будут использованы только первые 100.

{% endnote %}

{% note info "" %}

При передаче синонимов в бегемот, каждой фразе проставляется language из языка запроса.

{% endnote %}

### Использование в гранете

Чтобы изпользовать переданные сущности в [гранете](https://docs.yandex-team.ru/alice-scenarios/nlu/granet/), имеется конструкция `$ner.scenario.название_сущности`.
У слота должно быть два типа: `scenario.название_сущности` и `string`. Подробнее смотрите в [примерах](#examples_2) ниже.

### Как вернуть сущности из сценария

По аналогии с кнопками, сценарий может заполнить поле [ExternalEntitiesDescription](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/device_state.proto?rev=r9182611#L689) в ответе, которое будет преобразовано на мегамайнде в директиву и возвращено на клиент. Предполагается, что клиент связывает пришедшие сущности с полученным (или уже существующем) экраном (или элементом экрана) и отправляет их в запросе, при котором пользователь может взаимодействовать с экраном (или элементом экрана). Когда экран (или элемент экрана) удаляется из памяти, вместе с ним можно забыть и сущности, которые ему соответствовали.

### Примеры {#examples_2}

#### Айтемселекция на галерее с фильмами

Пусть имеется галерея фильмов из 4х элементов. Предположим, что пользователю видны только второй и третий фильмы. Чтобы реализовать айтемселекцию надо:

1. Добавить [гранет](https://docs.yandex-team.ru/alice-scenarios/nlu/granet/) вида:

```
form video.action.select_from_gallery:
    slots:
        action:
            type:
                scenario.video_name
                string
            source: $UserDefinedText

    root:
        $Open* $UserDefinedText

$Open:
    включи

$UserDefinedText:
    $ner.scenario.video_name
```

2. В мегамайнде создать соответствующий [TypedSemanticFrame](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/frame.proto?rev=r9154634#L1399). В нашем случае:

```

message TTypedSemanticFrame {
    ...
    TVideoActionSelectFromGallerySemanticFrame VideoActionSelectFromGallerySemanticFrame = 1111 [json_name = "video_action_select_from_gallery_semantic_frame", (NYT.column_name) = "video_action_select_from_gallery_semantic_frame"];
    ...
}

message TVideoNameSlot {
    option (NYT.default_field_flags) = SERIALIZATION_YT;

    oneof Value {
        string StringValue = 1 [json_name = "string_value", (NYT.column_name) = "string_value", (SlotType) = "scenario.video_name"];
    }
}

message TVideoActionSelectFromGallerySemanticFrame {
    option (NYT.default_field_flags) = SERIALIZATION_YT;
    option (SemanticFrameName) = "video.action.select_from_gallery";

    TVideoNameSlot Action = 1 [json_name = "action", (NYT.column_name) = "action", (SlotName) = "action"];
}
```

3. Сформировать нужный [DeviceState](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/device_state.proto?rev=r9168679#L119):

```json
{
    "device_state": {
        "external_entities_description": [
            {
                "items": [
                    {
                        "phrases": [
                            {
                                "phrase": "Гарри Поттер и философский камень"
                            },
                            {
                                "phrase": "Гарри Поттер часть 1"
                            },
                            {
                                "phrase": "Гарри Поттер 1"
                            },
                        ],
                        "value": {
                            "string_value": "movie_1"
                        }
                    },
                    {
                        "phrases": [
                            {
                                "phrase": "Гарри Поттер и Тайная комната"
                            },
                            {
                                "phrase": "Гарри Поттер часть 2"
                            },
                            {
                                "phrase": "Гарри Поттер 2"
                            },
                        ],
                        "value": {
                            "string_value": "movie_2"
                        }
                    },
                    {
                        "phrases": [
                            {
                                "phrase": "Гарри Поттер и узник Азкабана"
                            },
                            {
                                "phrase": "Гарри Поттер часть 3"
                            },
                            {
                                "phrase": "Гарри Поттер 3"
                            },
                        ],
                        "value": {
                            "string_value": "movie_3"
                        }
                    },
                    {
                        "phrases": [
                            {
                                "phrase": "Гарри Поттер и Кубок огня"
                            },
                            {
                                "phrase": "Гарри Поттер часть 4"
                            },
                            {
                                "phrase": "Гарри Поттер 4"
                            },
                        ],
                        "value": {
                            "string_value": "movie_4"
                        }
                    },
                ],
                "name": "video_name"
            }
        ],
        "active_actions": {
            "screen_conditional_action": {
                "main": {
                    "conditional_actions": [
                        {
                            "conditional_semantic_frame": {
                                "video_action_select_from_gallery_semantic_frame": {
                                    "action": {
                                        "string_value": "movie_2"
                                    }
                                }
                            },
                            "effect_frame_request_data": {
                                "analytics": {
                                    "origin": "some_origin",
                                    "origin_info": "some_origin_info",
                                    "product_scenario": "some_psn",
                                    "purpose": "some_purpose"
                                },
                                "origin": {...},
                                "typed_semantic_frame": {
                                    "video_play_semantic_frame": { /* данные для включения Гарри Поттер и Тайная комната */}
                                }
                            }
                        },
                        {
                            "conditional_semantic_frame": {
                                "video_action_select_from_gallery_semantic_frame": {
                                    "action": {
                                        "string_value": "movie_3"
                                    }
                                }
                            },
                            "effect_frame_request_data": {
                                "analytics": {
                                    "origin": "some_origin",
                                    "origin_info": "some_origin_info",
                                    "product_scenario": "some_psn",
                                    "purpose": "some_purpose"
                                },
                                "origin": {...},
                                "typed_semantic_frame": {
                                    "video_play_semantic_frame": { /* данные для включения Гарри Поттер и узник Азкабана */}
                                }
                            }
                        }
                    ]
                }
            }
        }
    }
}
```

Теперь по фразе "включи гарри поттер тайная комната" будет разобран фрейм ```video_action_select_from_gallery_semantic_frame``` со слотом ```action = movie_2```. Это приведет к срабатыванию кнопки с эффектом для включения "Гарри Поттер и Тайная комната".
Фраза "включи гарри поттер и философский камень" также приведет к разбору этого фрейма, но будет обрабатываться как обычный голосовой запрос без активации кнопок.

## Дополнительная информация
- [вики с примерами](https://wiki.yandex-team.ru/users/nkodosov/primery-raboty-s-golosovymi-knopkami/)
- [немного описания](https://wiki.yandex-team.ru/users/pazus/golosovye-knopki-na-jekrane/#ogranichenijaitrebovanija)
- [evo тесты с примерами](https://a.yandex-team.ru/arc/trunk/arcadia/alice/tests/integration_tests/megamind/test_active_actions.py?rev=r9040420#L1)
