Для использования добавления команды в контест (`AddCommand` в файле _{path_to_arcadia}/alice/bass/forms/context/context.h_) необходимо создать и зарегистрировать директиву в файле _{path_to_arcadia}/alice/bass/forms/directives.inc_

Формат:
```cpp
CREATE_AND_REGISTER_DIRECTIVE(TYourNameDirective, ANALYTICS_TAG_AS_STRING_BUF)
```

где:
* **TYourNameDirective**: имя класса. Он должен быть уникальным.
* **ANALYTICS_TAG_AS_STRING_BUF**: аналитический тег в виде текстового идентификатора. Он должен быть тоже уникальным. Он необходим для понимания, что произойдет, для дальнейшего анализа действий Алисы

Пример для объявлении и регистрации директивы:
```cpp
CREATE_AND_REGISTER_DIRECTIVE(TClientGoForwardDirective, "quasar_go_forward")
```

Теперь директиву можно применить.

Пример:
```cpp
context.AddCommand<TSoundMuteDirective>("sound_mute", NSc::Null());
```

Если команда динамически создается, то для этого необходимо воспользоваться функцией `GetAnalyticsTagIndex`:
```cpp
const auto index = GetAnalyticsTagIndex<TClientGoForwardDirective>();
context.AddCommand(commandName, index, NSc::TValue::Null());
```

После регистрации директивы, аналитический тег необходимо описать в файле _{path_to_arcadia}/alice/bass/data/directives.json_. Это необходимо для аналитиков, чтобы они понимали, какие действия производит Алиса помимо голосовых сообщений.

Описание формата _directives.json_:
* **analytics_tag** - аналитический тег в виде текстового идентификатора (то, что было указано в качестве _ANALYTICS_TAG_AS_STRING_BUF_). Он необходим для понимания, что произойдет, для дальнейшего анализа действий Алисы
* **type** - к какому типу относится команда. Варианты: _<client_action_type>_
* **author** - имя автора
* **ticket** - тикет сценария в трекера
* **human_description** - описание аналитического тега (что означает этот тег)


Пример:
```json
{
    "analytics_tag": "phone_send_message",
    "type": "<client_action_type>",
    "author": "igoshkin",
    "ticket": "ASSISTANT-3182",
    "human_description": "Отправка сообщения (смс или текст через мессенджер) указанному адресату"
}
```
