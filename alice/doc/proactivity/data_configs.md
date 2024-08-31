## Где лежат построллы
Все конфиги построллов лежат в директории [dj/services/alisa_skills/server/data](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data).
Основные файлы: 
* [proto_items/bases.pb.txt](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/proto_items/bases.pb.txt) и proto_items/<dir_name>/bases.pb.txt, например, [proto_items/music/bases.pb.txt](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/proto_items/music/bases.pb.txt)
* [proto_items/mods.pb.txt](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/proto_items/mods.pb.txt) и proto_items/<dir_name>/mods.pb.txt, например, [proto_items/music/mods.pb.txt](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/proto_items/music/mods.pb.txt)
* [promotions.pb.txt](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/promotions.pb.txt)
* [source_groups.pb.txt](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/source_groups.pb.txt).

В файлах bases.pb.txt и mods.pb.txt лежат сами построллы: их id, Result, Condition, SuccessCondition и тд. Некоторые построллы лежат в отдельных папочках, например, в [proto_items/music/bases.pb.txt](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/proto_items/music/bases.pb.txt) и [proto_items/music/mods.pb.txt](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/proto_items/music/mods.pb.txt) лежат музыкальные построллы. При необходимости можно создать новые тематические директории в [proto_items](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/proto_items). Неотсортированные построллы лежат в [proto_items/bases.pb.txt](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/proto_items/bases.pb.txt) и [proto_items/mods.pb.txt](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/proto_items/mods.pb.txt).

В [promotions.pb.txt](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/promotions.pb.txt) и [source_groups.pb.txt](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/source_groups.pb.txt) описывается таргетинг и места показов построллов.

Кроме этих конфигов есть [tag.pb.txt](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/tag.pb.txt) и [condition_presets.pb.txt](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/condition_presets.pb.txt), описывающие теги и Condition-ы построллов соответственно.

Далее подробней про каждый конфиг.

### Конфиги mods, bases и mods_parameters
На постролл с одним сценарием и смыслом может быть много текстов (например несколько построллов на включение звуков природы, они различаются только текстами) или несколько вариантов кнопок (например построллы на включение разных радиостанций "включи русское радио", "включи радио маяк" — у них различаются и кнопки, и тексты). В таком случае заводится один элемент в одном из bases.pb.txt файлов, например, в [proto_items/bases.pb.txt](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/proto_items/bases.pb.txt), характеризующий общие свойства и много в одном из mods.pb.txt файлов, например, в [proto_items/mods.pb.txt](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/proto_items/mods.pb.txt), различающихся по тексту и поведению.

Пример двух построллов ambient_sound с общей кнопкой и разными текстами.
В [proto_items/bases.pb.txt](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/proto_items/bases.pb.txt) — общая для построллов база
```bash
Items {
    Id: "ambient_sound" // уникальный ключ, читабельное название
    Tags: ["ambient_sound"] // тег из tags.pb.txt, определяет как часто может включаться постролл
    Condition: { 
        RequiredPresets: "has_yandex_plus",
        ExpFlags: "skillrec_ambient_sound_postroll"
    } // Необходимые условия для показа постролла
    PriorScore: 0.5 // То в каком приоритете будет включаться постролл, пока не наберет статистики и не научится сам это определять. Исторически — среднее значение ctr постролла
    Result: { 
        ShouldListen: true
        FrameAction { // Описание кнопки. У постролла без кнопки не будет FrameAction в Result (ни в bases, ни в mods).
            ParsedUtterance {
                TypedSemanticFrame {
                    MusicPlaySemanticFrame {
                        SearchText { StringValue: "звуки природы" }
                    }
                }
                Analytics {
                    Purpose: "play_ambient"
                }
            }
        }
    }
    Analytics: { // Что считать за успешное срабатывание постролла
        Info: "personal_assistant.scenarios.music_ambient_sound" // Описание сценария постролла, используется для агрегации показов, конверсий и таймспента построллов на борде и в метриках аб. Обычно сюда складывают интент постролла.
        SuccessConditions: {
            Frame { // считаем, что была конверсия, если пришел фрейм "personal_assistant.scenarios.music_play" и слот special_playlist со значением удовлетворяющим регулярке в Value 
                Name: "personal_assistant.scenarios.music_play"
                Slots: [{
                    Name: "special_playlist",
                    Value: "ambient_sounds_default|sea_sounds"
                }]
            }
        }
    }
    AvoidSource: "personal_assistant.scenarios.music.*" // Регулярка на интенты, после которых ЗАПРЕЩЕНО включать постролл. Там всегда должен быть интент постролла
}
```

В [proto_items/mods.pb.txt](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/proto_items/mods.pb.txt) — расширения для того что написано в базе, для каждого постролла (текста постролла) свой элемент
```bash
Items {
    Id: "ambient_sound__0" // Уникальное название в mods.pb.txt
    BaseItem: "ambient_sound" // Ключ из bases.pb.txt — наследуют его свойства
    Result: {
        Postroll: { // Текст постролла
            Voice: "Кстати, я могу включить вам звуки природы. Только скажите - и услышите шум моря перед сном."
        }
    }
}
Items {
    Id: "ambient_sound__1"
    BaseItem: "ambient_sound"
    Result: {
        Postroll: {
            Voice: "Нет ничего лучше шума моря. Если скучаете по нему - попросите меня включить звук волн."
        }
    }
}
```
#### mods_parameters
Часто айтемы в [mods.pb.txt](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/proto_items/mods.pb.txt) отличаются лишь несколькими полями (например, айтемы с разным текстом). Для этого есть файл [mods_parameters.json](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/proto_items/mods_parameters.json). Чтобы параметризовать айтем с Id ```ModName``` из [mods.pb.txt](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/proto_items/mods.pb.txt), в него нужно добавить поле ```NeedPlaceholderReplace: true``` и вставить плейсхолдеры в текстовые поля в формате ```{?placeholder_name}```. В файле [mods_parameters.json](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/proto_items/mods_parameters.json) нужно создать массив ```"ModName"``` с различными значениями плейсхолдеров. При этом итоговый айтем будет иметь Id ```ModName_{?index}```. Например, айтемы ```ambient_sound__0``` и ```ambient_sound__1``` из примера выше можно было бы записать так:

* mods.pb.txt:
```bash
Items {
    Id: "ambient_sound_" // Уникальное название в mods.pb.txt - заменится на "ambient_sound__{?index}"
    NeedPlaceholderReplace: true // Флаг, говорящий о необходимой замене плейсхолдеров, по дефолту false
    BaseItem: "ambient_sound" // Ключ из bases.pb.txt — наследуют его свойства
    Result: {
        Postroll: { // Текст постролла
            Voice: "{?postroll_voice}" // плейсхолдер
        }
    }
}
```

* mods_parameters.json:
```bash
{
    "ambient_sound_": [
        {
            "index": "0",
            "postroll_voice": "Кстати, я могу включить вам звуки природы. Только скажите - и услышите шум моря перед сном."
        }, 
        {
            "index": "1",
            "postroll_voice": "Нет ничего лучше шума моря. Если скучаете по нему - попросите меня включить звук волн."
        }
    ]
}
```

Пример параметризации построллов ```music_epochs_80_0``` и ```music_epochs_90_0```:
* mods.pb.txt:
```bash
Items {
    Id: "music_epochs"
    NeedPlaceholderReplace: true
    BaseItem: "music_epochs"
    Analytics: {
        SuccessConditions: [{
            Frame: {
                Name: "personal_assistant.scenarios.music_play"
                Slots: [{
                    Name: "epoch"
                    Value: "{?epoch_value}"
                }]
            }
        }]
    }
    Result: {
        Postroll: {
            Voice: "{?voice}"
        }
        FrameAction: {
            ParsedUtterance: {
                TypedSemanticFrame {
                    MusicPlaySemanticFrame {
                        Epoch { EpochValue: "{?epoch_value}" }
                    }
                }
            }
        }
    }
}
```

* mods_parameters.json:
```bash
{
    "music_epochs": [
        {
            "index": "80_0",
            "epoch_value": "eighties",
            "voice": "текст 1"
        },
        {
            "index": "90_0",
            "epoch_value": "nineties",
            "voice": "текст 2"
        }
    ]
}
```
Обратите внимание, что один плейсхолдер можно использовать несколько раз в одном айтеме.

### Конфиг promotions
В [promotions.pb.txt](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/promotions.pb.txt) описывается таргетинг построллов.
Чтобы сервис "видел" постролл и мог его показать, id постролла необходимо добавить в один из promotion-ов.
Добавлять можно или в уже существующие promotion-ы, или завести новый, если среди существующих нет подходящего таргетинга.
Один постролл может быть указан сразу в нескольких promotion-ах. Например, в promotion-ах morning (показы утром) и day (показы днем).

Примеры существующих promotion-ов:

* Promotion без дополнительных условий:
```bash
Promotions {
    Id: "common"
    Items: [
        "ambient__[0-4,9]",
        "external_skill__common_games_[0,9]"
    ]
}
```

* Promotion с условием, что показываем только на детском голосе:
```bash
Promotions {
    Id: "for_children"
    Condition: { RequiredPresets: "is_child" }
    Items: [
        "external_skill__for_children_[5,7]",
        "music_children_[0,5]",
    ]
}
```

* Promotion с условием, что показываем только днем и только после source-ов, удовлетворяющих регулярке в Source:
```bash
Promotions {
    Id: "day_after_hello"
    Condition: { RequiredPresets: "day" }
    Source: "personal_assistant.handcrafted.*"
    Items: ["morning_show__hello_day_[0-2]"]
}
```


### Конфиг source_groups
В [source_groups.pb.txt](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/source_groups.pb.txt) 
описываются source-ы, места приклеивания построллов.
Построллы могут клеиться только к source-ам, указанным в source_groups.pb.txt.

Source-ы хранятся объединенные в группы SourceGroups. 
В SourceGroup-е перечисляются интенты, к которым будет клеиться постролл, и дополнительные условия на показ ApplyCondition. 
```bash
SourceGroups {
    Id: "not_listening"
    Ids: [ // Интенты, к которым будем клеить построллы
        "personal_assistant.scenarios.create_reminder",
        "personal_assistant.scenarios.timer_set"
    ]
    ApplyCondition: { Listening: { value: false } } // Условие, что для приклеивания постролла ответ сценария должен быть без дослушивания.
}
```

### Конфиг tags.pb.txt
В [tags.pb.txt](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/tags.pb.txt) описываются теги построллов.
Тег задает ограничение по количеству построллов, времени или запросам в между показами двух построллов с одинаковым набором тегов. 
Они нужны, чтобы построллы на одну тему не показывались подряд. Чаще всего используются теги с ограничением на количество построллов.
Например, такой тег search
```bash
Tags {
  Id: "search"
  Description: "personal_assistant.scenarios.search"
  PostrollDeltaVariants {
      DeltaSinceLast: 5
  }
}
```
означает, что между любыми двумя построллами с тегом "search" должно быть показано 5 построллов с другими тегами.

В случае, если у постролла есть несколько тегов, то берется максимальный их них. Это можно использовать для распределения показов между вашими построллами.
Например, можно указать теги ```music=4, music_genres=9``` у построллов жанров музыки и теги ```music=4, music_artists=9``` у построллов музыкальных исполнителей.
Тогда каждый 5й постролл будет музыкальным, и половина из них будет про жанры музыки, а половина — про исполнителей.


### Конфиг condition_presets.pb.txt
В [condition_presets.pb.txt](https://a.yandex-team.ru/arc/trunk/arcadia/dj/services/alisa_skills/server/data/condition_presets.pb.txt) 
описываются Condition-ы, которые потом можно использовать в bases, mods и promotions. 
Если ваш Condition используется в нескольких местах, его стоит положить в condition_presets.pb.txt. 
После добавления условия в condition_presets.pb.txt им можно воспользоваться, указав его id в поле RequiredPresets или RequiredAnyPresets внтури Condition. 

Пример пресета ```has_yandex_plus```. Определяем сам пресет:
```bash
ConditionPresets {
    Id: "has_yandex_plus"
    Check {
        Expression: "UserInfo.HasYandexPlus"
    }
}
```
и указываем его в base постролла:
```bash
Items {
    Id: "music_with_button__playlists"
    Tags: "music"
    Tags: "music_playlists"
    Condition {
        RequiredPresets: "has_yandex_plus"
    }
    PriorScore: 0.6
    Analytics {
        Info: "personal_assistant.scenarios.music_play"
    }
}
```
#### RequiredPresets и RequiredAnyPresets {#required_presets}
Внутри Condition и ConditionPresets можно указывать другие пресеты. Пресеты можно использовать через ```RequiredPresets```, где пресеты берутся через логическое И, или через ```RequiredAnyPresets```, где пресеты берутся через логическое ИЛИ. Между собой условия ```RequiredPresets``` и ```RequiredAnyPresets``` берутся через логическое И. Например, пресет ```test_preset``` эквивалентен ```(preset_1 AND NOT preset_2) AND (preset_3 OR preset_4)```:
```bash
ConditionPresets {
    Id: "test_preset"
    RequiredPresets: ["preset_1", "!preset_2"]
    RequiredAnyPresets: ["preset_3", "preset_4"]
}
```

Реальные примеры:
* Пресет [late](https://a.yandex-team.ru/arcadia/dj/services/alisa_skills/server/data/condition_presets.pb.txt?rev=r9558095#L55) выполняется при выполнении хотя бы одного из пресетов [evening](https://a.yandex-team.ru/arcadia/dj/services/alisa_skills/server/data/condition_presets.pb.txt?rev=r9558095#L43), [late_evening](https://a.yandex-team.ru/arcadia/dj/services/alisa_skills/server/data/condition_presets.pb.txt?rev=r9558095#L49) и [night](https://a.yandex-team.ru/arcadia/dj/services/alisa_skills/server/data/condition_presets.pb.txt?rev=r9558095#L25).
* Пресет [old_user_with_used_alarm](https://a.yandex-team.ru/arcadia/dj/services/alisa_skills/server/data/condition_presets.pb.txt?rev=r9558095#L990) выполняется при выполнении пресета [old_user](https://a.yandex-team.ru/arcadia/dj/services/alisa_skills/server/data/condition_presets.pb.txt?rev=r9558095#L971) и [своего check](https://a.yandex-team.ru/arcadia/dj/services/alisa_skills/server/data/condition_presets.pb.txt?rev=r9558095#L992).
* Condition из [evening_show](https://a.yandex-team.ru/arcadia/dj/services/alisa_skills/server/data/proto_items/alice_show/bases.pb.txt?rev=r9558227#L46) выполняется при одновременном выполнении пресета ```alice_show_allowed``` и невыполнении пресета ```has_used_morning_show_last_2_days```, так как ```!``` в начале означает отрицание.