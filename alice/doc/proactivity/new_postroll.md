## Пайплайн заведения постролла
1. Счекаутить себе Аркадию. Как это сделать, можно прочитать в [инструкции](https://docs.yandex-team.ru/devtools/intro/quick-start-guide).
2. Забрать себе последнюю версию конфигов построллов: ```arcadia/dj/services/alisa_skills/server/data``` (как это сделать, написано в той же [инструкции](https://docs.yandex-team.ru/devtools/intro/quick-start-guide)).
3. Написать построллы по инструкции ниже.
4. Протестировать построллы

   4.1 Если умеете поднимать сервис проактивности, то протестировать (послушать построллы, согласиться на кнопки) локально по [инструкции](testing#testing_local).

   4.2 Если протестировать локально не получается, то построллы надо убрать под флаг и, когда они доедут до прода, протестировать с этим флагом по [инструкции](testing#testing).
   Если все ок, то можно закоммитить удаление флага и построллы включатся в проде на всех.

4. Создать с пулл-реквест с вашими изменениями, [инструкция](https://docs.yandex-team.ru/devtools/src/arc/workflow#workflow).
5. Найти ваш пулл-реквест на [a.yandex-team.ru](https://a.yandex-team.ru/reviews/outgoing?filterOrder=s&sort=-lastUpdated&status=pending) в разделе Outgoing Reviews.
6. Дождаться пока пробегут тесты или прогнать тесты локально перед созданием PR и перейти к следующему пункту.
7. Запаблишить ревью кнопкой Publish.
8. Пройти ревью и получить Ship от одного из [ревьюверов](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/.devexp.json?rev=r8833110#L24-26).
9. Прожать кнопку Merge, чтобы замержить изменения.
10. Замерженные изменения появятся на проде в течение двух часов.


## Написание построллов
1. Понять, что вы хотите промотировать и с каким таргетингом.  Для текстов построллов создать тикет на [@tuluzza](https://staff.yandex-team.ru/tuluzza) в очередь [ALICETXT](https://st.yandex-team.ru/ALICETXT).
2. Определить тип постролла: с голосовой кнопкой или нет. Подробнее про то, как [писать кнопки](#postroll_buttons).
3. Определить условие успешного срабатывания ```SuccessConditions```. Подробнее про то, как [писать SuccessConditions](#success_conditions).
4. Найти подходящую тематическую папку в [proto_items](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/proto_items). Если таковая есть, то добавить постролл в соответсвующий bases.pb.txt и mods.pb.txt. Если таковой нет, то можно добавить постролл в общие файлы [proto_items/bases.pb.txt](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/proto_items/bases.pb.txt) и [proto_items/mods.pb.txt](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/proto_items/mods.pb.txt). Если существует желание создать новую тематическую папку, то нужно создать новую директорию с файлами bases.pb.txt и mods.pb.txt, добавить построллы туда, после чего добавить новые файлы на вход скриптов в [proto_items/ya.make](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/proto_items/ya.make): [сюда](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/proto_items/ya.make?rev=r9069965#L10) и [сюда](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/proto_items/ya.make?rev=r9069965#L12) дописать <dir_name>/bases.pb.txt, [сюда](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/proto_items/ya.make?rev=r9069965#L18) и [сюда](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/proto_items/ya.make?rev=r9069965#L20) дописать <dir_name>/mods.pb.txt.
5. Выбрать или создать новый таргетинг и добавить в него свои фразы в [promotions.pb.txt](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/promotions.pb.txt).
    1. Скорее всего нужный вам таргетинг уже есть. Посмотрите на существующие promotions, прежде чем заводить новый. Что уже есть:
       * Три места, если вы хотите построллить максимально широко:
         * ```common``` — показывается везде; можно добавлять фразы только если вы уверены, что их можно показывать и взрослым и детям
         * ```for_adults``` — показывать везде взрослым (по биометрии), **самое место** для фраз, про которые вы не уверены, что стоит показывать детям
         * ```for_children``` — показывать везде детям (по биометрии), если вы знаете, что взрослым ваше предложение будет точно не интересно
       * Таргетинг на время суток - промоушены ```morning, day, etc```
       * Таргетинг на время суток + место прикрепления постролла - промоушены ```morning_after_hello, night_after_alarm, etc```
    2. Если нужен новый таргетинг, то подумать, не является ли ваше условие показа верным для всех возможных ситуаций, в которых Алиса должна произнести постролл (например, наличие плюса для рекомендаций музыки или наличие у пользователя умной лампочки для рекомендаций новой фичи УД).
    Если вы придумали условие, которое выполняется для всех показов, то его имеет смысл написать в ```Condition``` в соответствующий ```mod``` или ```base```.

## Голосовые кнопки {#postroll_buttons}
Чтобы добавить кнопку в постролл, нужно положить поле [TFrameAction](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/scenarios/response.proto#L54) в поле ```Result```.
Внутри ```TFrameAction``` необходимо указать фрейм, который будет запускаться по кнопке.
Сейчас правильно использовать [ParsedUtterance](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/scenarios/frame.proto#L18) с указанием TypedSemanticFrame.
Если для сценария ваших построллов еще нет типизированного фрейма, то нужно сначала его завести.

Пример ```FrameAction``` для запуска плейлиста дня:
```bash
FrameAction {
    ParsedUtterance {
        TypedSemanticFrame {
            MusicPlaySemanticFrame {
                SpecialPlaylist {
                    SpecialPlaylistValue: "playlist_of_the_day"
                }
            }
        }
        Analytics {
            Purpose: "play_music_playlist"
        }
    }
}
```
Поле ```Analytics.Purpose``` -- произвольное поле с описанием действия кнопки.

Также у ```TFrameAction``` можно указать [NluHint](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/frame.proto#L66) с описанием фраз, на которые будет срабатывать кнопка.
По умолчанию кнопка активируется грамматикой [alice.proactivity.confirm](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/granet/proactivity/confirm.grnt#L8)
(фразы типа "да", "давай", "включи").

### Как понять, какой фрейм указывать
1. Сначала нужно определить фрейм вашего сценария.
Для этого можно задать запрос в Алису и посмотреть на фреймы в ответе Бегемота или воспользоваться [тулзой](tools#tool_frame_grep).
2. Поискать [здесь](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/frame.proto?rev=r8926877) соответствующий типизированный фрейм (по полю ```SemanticFrameName```).
3. Если нашли нужный фрейм, то указываете его в поле ```FrameAction.ParsedUtterance.TypedSemanticFrame```, и можно переходить к пункту 6.
4. Если нужного фрейма нет, то надо завести его самому либо попросить об этом разработчиков сценария.
5. Дождаться релиза ММ и SkillRec-а с этим фреймом.
   * Если по каким-то причинам нет возможности ждать релизов ММ и SkillRec,
   то можно в ```ParsedUtterance``` указать нужный [Frame](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/frame.proto#L1335)
   и добавить его название в [whitelist](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/recommender/components/proactivity_ut.cpp?rev=r8908694#L40).
   Но нужно иметь в виду, что поле ```Frame```  deprecated и в какой-то момент ваша кнопка перестанет работать.
6. Добавить и протестировать кнопку.


## Condition {#condition}
### Куда писать новые условия?
Один и тот же Condition постролла можно указать в разных местах: mods, bases, promotions.
Чтобы понять, куда лучше написать ваш  Condition, можно руководствоваться следующими правилами:
* ```promotions``` vs ```bases/mods```

  Если постролл можно показывать без данного условия, то условие должно быть в promotions. Иначе — в bases или mods.
  Примеры для постролла ```Включить вам музыку?```
    * Условие "только пользователю с Плюсом": без плюса музыку не включить — кладем условие в ```bases/mods```.
    * Условие "только утром": музыку можно включать и вечером — кладем условие в ```promotions```.
* ```bases``` vs ```mods```

  Если условие нужно всем построллам из base, который вы добавляете, то лучше положить его в base. Иначе — только в mods, где оно нужно.
  Пример для условия на плюс:
    * Для построллов музыки лучше положить в base, так как оно нужно всем музыкальным построллам.
    * Для построллов УШ надо складывать только в те mods, где в тексте упоминается музыка.
    Так как сейчас для пользователей без плюса работает УШ без музыки.
* ```condition_presets```

  Стоит положить ваш Condition в файлик [condition_presets.pb.txt](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/condition_presets.pb.txt), если он используется нескольких местах, чтобы избежать лишнего дублирования кода.

### Как написать Condition
В Condition можно задавать следующие поля:
* [Флаги эксперимента](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/proto_items/bases.pb.txt?rev=r8921022#L110-111)
```bash
Condition {
    ExpFlags: "skillrec_whisper_turn_on_postrolls"
    ExpFlags: "mm_enable_protocol_scenario=Voice"
}
```
* [Регулярка на поверхность](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/proto_items/bases.pb.txt?rev=r8921022#L8721)
```bash
Condition {
    App: "^ru.yandex.quasar.*"
}
```

* [Список](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/proto_items/bases.pb.txt?rev=r8921022#L563-566) других Condition-ов (пресетов), определенных в [condition_presets](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/condition_presets.pb.txt).
```bash
Condition {
    RequiredPresets: "has_yandex_plus"
    RequiredPresets: "is_child"
}
```
Пресеты можно брать через RequiredPresets (логическое И) и через RequiredAnyPresets (логическое ИЛИ). Подробнее про [RequiredPresets/RequiredAnyPresets](data_configs#required_presets).
* Универсальная проверка Check, позволяющая писать условия на значения в протобуфах запроса. Подробнее про [написание Check-ов](#condition_check).
* Попадание в [группу puid-ов](#puid_groups).


### Как написать Check в Condition {#condition_check}
[Wiki](https://wiki.yandex-team.ru/alice/skillrec/checks/) про синтаксис Check-ов.

Check-и работают на основе библиотеки [library/cpp/expression](https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/expression)
и позволяют вычислять выражения с использованием данных из протобуфов и предопределенных параметров из контекста запроса.
Можно писать условия на предопределенный параметр ```MinutesFromMidnight``` (время с начала дня в минутах)
и на протобуфы ```UserInfo```, ```DeviceState```, ```PersonalIntents```, ```UserClassification```, etc.
Весь список можно посмотреть [здесь](https://wiki.yandex-team.ru/alice/skillrec/checks/#dostupnyeprotobufyipredopredeljonnyeparametry).

Также в выражении можно использовать значения флагов экспериментов. Название параметра соответствует названию флага с префиксом ```exp_```.
Если флага нет, то подставляется пустая строка, которая интерпретируется как ложь в булевских выражениях.
Если флаг есть, то подставляется 1 или значение после знака "равно", если он есть во флаге.


В простом случае в Check-е надо задать только поле Expression и написать в нем выражение с использованием полей в протобуфах и/или предопределенных параметров.

Пример Check-а на наличие плюса:
```bash
ConditionPresets {
    Id: "has_yandex_plus"
    Check {
        Expression: "UserInfo.HasYandexPlus" // Вычисляем значение поля HasYandexPlus в протобуфе UserInfo.
    }
}
```

Пример с предопределенным параметром ```MinutesFromMidnight``` (время суток в минутах).
```bash
ConditionPresets {
    Id: "day"
    Check {
        Expression: "720 <= MinutesFromMidnight && MinutesFromMidnight < 1080" // Проверяем, что сейчас от 12 до 18 часов.
    }
}
```

Пример с флагом эксперимента ```skillrec_debug_text_response```:
```bash
ConditionPresets {
    Id: "voice_input"
    Check {
        Expression: "Event.Type == \"voice_input\" || exp_skillrec_debug_text_response"
    }
}
```

Пример с флагами экспериментов ```skillrec_day_begin=720 skillrec_day_end=1080``` (можно поставить любое число после знака равенства):
```bash
ConditionPresets {
    Id: "day"
    Check {
        Expression: "(~exp_skillrec_day_begin ? exp_skillrec_day_begin : 720) <= MinutesFromMidnight &&"
                    " MinutesFromMidnight < (~exp_skillrec_day_end ? exp_skillrec_day_end : 1080)"
    }
}
```

Также можно писать более сложные условия, когда внутри Check вы определяете свои параметры для использования в ```Expression```.
В параметре задается имя ```Name``` и путь в протобуфе ```Path```, а параметр принимает значение, извлеченное из протобуфа по указанному пути.
Если указано правило агрегации ```Aggregate```, то значение получается путем перебора повторяемого поля протобуфа и агрегацией по заданным правилам.

При использовании ```Aggregate``` надо помнить, что перебор идет линейно. Это негативно сказывается на производительности сервиса, если перебирается много значений.

#### Пример Check-а с кастомными параметрами, но без агрегации
Проверка, что запрос распознался как детский и был задан шепотом:
```bash
ConditionPresets {
    Id: "child_whisper_request"
    Check {
        Expression: "UserAge == \"Child\" && IsWhisper" // Проверяем, что это детский запрос шепотом
        Parameters {
            Name: "UserAge" // Параметр называется UserAge
            Path: "UserClassification.Age" // Значение параметра достаем из поля UserClassification.Age
        }
        Parameters {
            Name: "IsWhisper" // Параметр называется IsWhisper
            Path: "ProactivityRequest.WhisperInfo.IsAsrWhisper" // Значение параметра достаем из поля ProactivityRequest.WhisperInfo.IsAsrWhisper
        }
    }
}
```



#### Примеры Check-ов с агрегацией
* Проверка, что в запросе присутствует фрейм ```personal_assistant.scenarios.video_play```:
```bash
ConditionPresets {
    Id: "video_play_frame"
    Check {
        Expression: "HasVideoPlayFrame" // Значение выражения равно значению параметра HasVideoPlayFrame
        Parameters {
            Name: "HasVideoPlayFrame"
            Path: "ProactivityRequest.ScenarioToFrames[\"Video\"].SemanticFrames" // Итерируемся по списку фреймов
            Aggregate {
                Reducer: Or // Тип агрегации, в данном случае HasVideoPlayFrame=true, если хотя бы один из фреймов имеет имя "personal_assistant.scenarios.video_play"
                Value {
                    Expression: "SemanticFrames.Name == \"personal_assistant.scenarios.video_play\"" // Проверяем имя фрейма
                }
            }
        }
    }
}
```


* Проверка отсутствия подписки с ```Id="1"```:
```bash
ConditionPresets {
    Id: "not_subscribed_to_digest_subscription"
    Check {
        Expression: "!HaveSubscription" // Проверяем не нашлось ни одной подписки с Id=1
        Parameters {
            Name: "HaveSubscription"
            Path: "NotificationState.Subscriptions" // Итерация по списку подписок
            Aggregate {
                Reducer: Or // Проверяем, что есть хотя бы одна подписка с Id=1
                Value {
                    Expression: "Subscriptions.Id == \"1\"" // Проверка Id подписки
                }
            }
        }
    }
}
```

* Проверка, что пользователь пользовался сценарием сказок больше 3х раз:
```bash
ConditionPresets {
    Id: "has_used_fairy_tale_many_times"
    Check {
        Expression: "HasUsedFairyTaleManyTimes"
        Parameters {
            Name: "HasUsedFairyTaleManyTimes"
            Path: "PersonalIntents.Intents" // Итерируемся по списку интентов пользователя
            Aggregate {
                Reducer: First
                Filter {
                    Expression: "Intents.Intent == \"personal_assistant.scenarios.music_fairy_tale\"" // Проверяем, что это интент сказок
                }
                Value {
                    Expression: "Intents.Count >= 3" // пользовались хотя бы три раза
                }
            }
        }
    }
}
```

* Проверка, что пришел музыкальный фрейм без фильтров и ondeman-ов
```bash
ConditionPresets {
    Id: "music_frame_without_filters"
    Check {
        Parameters {
            Path: "ProactivityRequest.ScenarioToFrames[\"HollywoodMusic\"].SemanticFrames"
            Aggregate {
                Reducer: First
                Filter { // Рассматриваем только фрейм music_play
                    Expression: "SemanticFrames.Name == \"personal_assistant.scenarios.music_play\""
                }
                Value {
                     // Допускаем только слот action_request и слоты с опциями проигрывания
                    Expression: "HasOnlyPersonalityAndPlayOptionsSlots"
                    Parameters {
                        Name: "HasOnlyPersonalityAndPlayOptionsSlots"
                        Path: "SemanticFrames.Slots"
                        Aggregate {
                            Reducer: And
                            Value: {
                                Expression: "Slots.Name == \"action_request\""
                                            "|| Slots.Name == \"repeat\""
                                            "|| Slots.Name == \"order\""
                                            "|| Slots.Name == \"room\""
                                            "|| Slots.Name == \"location\""
                            }
                        }
                    }
                }
            }
        }
    }
}
```


## SuccessCondition {#success_conditions}
SuccessCondition описывает, какие запросы считать конверсией постролла. Обычно это условия на фреймы запроса.
Чтобы считать конверсии построллов, для каждого пользователя (uuid-а) хранятся последние 5 показанных постролла и их SuccessCondition-ы.
На каждом запросе проверяются все сохраненные SuccessCondition-ы и, в случае срабатывания одного из них, засчитывается клик соответствующему построллу.
В случае, когда у постролла несколько SuccessCondition, при проверке конверсии они берутся через логическое ИЛИ.


Далее клики используются в расчете конверсии и таймспента, приносимого построллом, по которым потом ранжируются построллы.
Поэтому у каждого постролла должен быть хотя бы один SuccessCondition.

### Что можно писать в SuccessCondition
Основных поля два -- это ```Frame``` и ```Check``` (тот же, что используется в Condition).
В поле ```Frame``` можно указать необходимый фрейм и регулярки на значения слотов.
SuccessCondition применяется к фреймам из ответа Бегемота победившего сценария.
Например, следующее условие
```bash
SuccessConditions {
    Frame: {
        Name: "personal_assistant.scenarios.music_play"
        Slots: [{
            Name: "search_text"
            Value: "(шум|звук).+камин"
        }]
    }
}
```
будет срабатывать на всех запросах, где в разборе есть фрейм ```personal_assistant.scenarios.music_play```, содержащий слот ```search_text``` со значением, удовлетворяющим регулярке ```(шум|звук).+камин```.

**Важно**
* Регулярки на слот матчатся как подстрока, то есть ```slot_value``` эквивалентно ```.*slot_value.*```.
* Сейчас SuccessCondition не поддерживает условия на отсутствие слота.


### Алгоритм написания SuccessCondition
#### С использованием ```Frame```
1. Сначала надо понять, какие фреймы приходят в запросах, на которых ожидается срабатывание SuccessCondition-а.
Это можно сделать тремя способами:
   * Воспользоваться [тулзой](tools#tool_frame_grep).
   * Грепнуть в [логах](//home/alice/dialog/prepared_logs_expboxes) запросы, на которых вы ожидаете конверсии постролла.
   Фреймы будут лежать в  ```analytics_info.modifiers_info.proactivity.semantic_frames_info```.
   * Задать в Алису запрос с флагом ```analytics_info```, на котором должна быть конверсия.
   Нужные фреймы будут лежать в ответе ММ в ```analytics_info.modifiers_info.proactivity.semantic_frames_info```.
2. Пишем условие на найденные фреймы в SuccessCondition.
3. Если умеете собирать код локально, можно протестировать написанный SuccessCondition [тулзой](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/tools/evo_generator).
Ей надо передать урл локально поднятого сервиса проактивности, и она проверит срабатывания SuccessCondition-ов у заданных id построллов.
   * Если у постролла есть кнопка, то тулза проверит, что при нажатии на кнопку срабатывает SuccessCondition.
   * Если кнопки у постролла нет или хочется проверить SuccessCondition не только на кнопке, но и на прямых запросах в Алису,
   то нужно в постролл положить примеры запросов, на которых ожидается срабатывание SuccessCondition-а, [пример](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/proto_items/mods.pb.txt?rev=8275534&blame=true#L812).

#### Примеры
* Простой пример для постролла будильника, за успех считаем запрос с фреймом ```personal_assistant.scenarios.alarm_set```.
```bash
SuccessConditions {
    Frame {
        Name: "personal_assistant.scenarios.alarm_set"
    }
}
```

* SuccessCondition для постролла звуков природы с условием на слот search_text.
```bash
SuccessConditions {
    Frame: {
        Name: "personal_assistant.scenarios.music_play"
        Slots: [{
            Name: "search_text"
            Value: "(шум|звук).+камин"
        }]
    }
}
```

* Пример с несколькими SuccessCondition-ами.
Засчитываем конверсию звукам природы, если пришел фрейм ```personal_assistant.scenarios.music_ambient_sound```,
либо пришел фрейм ```personal_assistant.scenarios.music_play``` со слотом ```special_playlist=ambient_sounds_default```:
```bash
SuccessConditions: [{
    Frame {
        Name: "personal_assistant.scenarios.music_ambient_sound"
    }
}, {
    Frame {
        Name: "personal_assistant.scenarios.music_play"
        Slots: [{
            Name: "special_playlist",
            Value: "ambient_sounds_default"
        }]
    }
}]
```

#### С использованием ```Check```
Check в SuccessCondition работает аналогично Check-у в Condition-е.
Основное отличие Check-а в SuccessCondition и в Condition в том, что в них доступны разные данные из запроса.
Подробнее про [написание Check-ов](#condition_check) и [список](https://wiki.yandex-team.ru/alice/skillrec/checks/#dostupnyeprotobufyipredopredeljonnyeparametry) доступных для проверки полей.

* SuccessCondition для постролла про активацию Плюса. Его конверсией считаем, что у пользователя появился Плюс:
```bash
SuccessConditions {
    Check {
        Expression: "UserInfo.HasYandexPlus"
    }
    IsTrigger: true
}
```
Поле ```IsTrigger: true``` означает, что мы засчитываем только первую конверсию постролла.


## Таргетинг по списку puid-ов {#puid_groups}
Позволяет показывать ваш постролл только заданному списку пользователей. Такие построллы называются групповыми.
[Пример](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/proto_items/mods.pb.txt?rev=r8947585#L8146).

Сервис хранит данные вида ```(group_id — [puid_1,...puid_n])``` со списками puid-ов, входящих в каждую группу.
Групповые построллы лежат в конфиге с обычными построллами с указанием в Condition-е группы, которой они показываются:

```bash
Condition: {
    UserGroups: ["personal_music_group_0"]
}
```
Для добавления нового списка puid-ов требуется релиз сервиса.

Как добавить групповые построллы:
1. Собрать табличку пар ```(puid -- group_id)```.
2. Отдать ее [@karina-usm](https://staff.yandex-team.ru/karina-usm) для добавления в конфиг сервиса.
3. Дождаться релиза SkillRec с новыми данными.
4. После релиза сервис будет знать про новые группы пользователей и можно добавлять построллы для этих групп.
