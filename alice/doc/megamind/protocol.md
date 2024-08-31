# Протокол работы с Мегамайндом

Чтобы реализовать протокол обмена репликами с Мегамайндом, нужно:

* Разобраться с тем, как следует [вызывать ваш сценарий](../architecture.md#variants).
* Корректно обрабатывать [запросы](#request), которые присылает Мегамайнд.
* Возвращать [ответы](#response) правильного формата.


## Типы сценариев {#scenario-types}

Для Мегамайнда есть два типа сценариев: `AppHostPure` (предпочтительный) и `AppHostProxy` (deprecated). К первым относятся все сценарии, имеющие свои аппхостовые графы, то есть те сценарии, поход в которые может быть организован, как поход в подграф, в частности, к этому типу относятся все сценарии, реализованные на Голливуде. Ко второму типу относятся сценарии, имеющие свой http сервер и отвечающие на http запросы.

NB: Помимо способа отправки запроса, есть отличие в послании датасорсов: для `AppHostProxy` они приходят в теле запроса, для `AppHostPure` они лежат отдельными айтемами в контексте.


## Авторизация {#auth}

В протоколе взаимодействия Мегамайнд использует авторизацию с помощью [TVM 2](https://wiki.yandex-team.ru/passport/tvm2/): в каждом запросе сценарий получает заголовок с сервисным тикетом (`X-Ya-Service-Ticket`) и с тикетом пользователя, если пользователь авторизован (`X-Ya-User-Ticket`).

Идентификаторы TVM-приложений Мегамайнда:

* тестинг: `2000860`
* продакшен: `2000464`


## Необходимые ручки {#necessary}

Чтобы понять, какие именно ручки вам нужно реализовывать, определитесь с возможные вариантами работы сценария:

Вариант | Ручки | Пример
--- | --- | ---
чистый | `/run` | Запрос погоды (теоретически не требует сайд-эффектов).
чистый, но медленный | `/run` + `/continue`| Запрос музыки (`/run` возвращает результаты поиска, `/continue` — музыкальный трек).
окончательный ответ при ранжировании, сайд-эффект потом | `/run` + `/commit` | Покупка фильма. Ответ про «оплатите в приложении» можно сформировать до того, как отправить пуш про оплату.
окончательный ответ после ранжирования и сайд-эффекта | `/run` + `/apply` | Внешние навыки (Диалоги). Невозможно составить ответ, пока внешний навык не ответил.

О том, что должны возвращать нужные вам ручки, читайте в разделе [{#T}](#response).

## Время ответа и ретраи {#timeout}

Ответ каждой ручки должен укладываться в 300 мс (стандартный таймаут Мегамайнда).

Время ответа считается не для реплики в целом, а для каждого вызова ручки: если Мегамайнд вызывает сначала ручку `/run`, а потом, например, ручку `/commit`, у сценария есть в сумме 600 мс, чтобы сформулировать ответ. Этим можно воспользоваться, чтобы распределить во времени операции для обработки реплики, которые не укладываются в 300 мс.

{% note info %}

Мегамайнд может ретраить ручки без сайд-эффектов (`/run` и `/continue`).

{% endnote %}

Если ваши ручки никак не укладываются в 300 мс, приходите с графиками посоветоваться c [zubchick@](https://staff.yandex-team.ru/zubchick).


## Формат данных {#format}

Взаимодействие между Мегамайндом и сценариями происходит только по
протоколу HTTP. Мегамайнд отправляет запросы и принимает ответы только
в формате Binary Protobuf.

Чтобы было удобнее отлаживать, поддержите JSON или ProtoText на бекенде сценария и пишите в логи текстовое представление запросов и ответов — это сильно поможет в тестировании логики.


## Запрос {#request}

Запросы от Мегамайнда приходят с телом в формате ??

Варианты запросов от Мегамайнда описаны элементами `TScenarioRunRequest` и `TScenarioApplyRequest` в файле [request.proto](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/scenarios/request.proto):

* `TScenarioRunRequest` описывает первый запрос к ручке `/run` после того, как сценарий был отобран для ранжирования.
* `TScenarioApplyRequest` описывает запросы к ручкам, которые вызываются при победе в ранжировании: `/commit`, `/apply` и `/continue`.




## Ответ {#response}

{% note warning %}

Мегамайнд рассчитывает, что сценарий всегда отвечает кодом `200 OK`, а необходимую информацию возвращает в теле ответа.

{% endnote %}

Все HTTP-коды ответа кроме `200 OK` могут интерпретироваться как ошибка в сценарии. Чтобы Мегамайнд перестал (или не начинал) [ретраить](#timeout) ручку `/run`, сценарий может вернуть ответ с кодом `429 (Too Many Requests)`.

Шпаргалка по возможным последовательностям вызовов:

Первый вызов | Ответ на первый вызов | Второй вызов | Ответ на второй вызов
--- | --- | --- | ---
`/run` | `ResponseBody` | — | —
`/run` | `CommitCandidate` + `ApplyArguments`| `/commit` | `success` или `error`
`/run` | `ContinueArguments` | `/continue` | `ResponseBody`
`/run` | `ApplyArguments` | `/apply` | `ResponseBody`


### Ручка для ранжирования (/run) {#run}

Ответ ручки `/run` должен содержать всю информацию об ответе сценария пользователю, которую сценарий может предоставить на этапе ранжирования. Общая структура ответа описана в спецификации элемента [TScenarioRunResponse](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/scenarios/response.proto).


Для каждого сценария ответ ручки `/run` должен включать:

* Свойство `Features`, с описанием фич ответа для постклассификатора.

  Если сценарий считает, что не подходит для ответа на реплику, в
  ответе правильно передать фичу `IsIrrelevant`. При этом в теле
  ответа обязательно нужно поместить ответ для пользователя
  объясняющий причину отказа в работе в `ResponseBody`, на случай если
  нерелевантным оказался единственный опрошенный сценарий.

  Например, сценарий вызова внешнего навыка Диалогов может ответить «у меня нет такого навыка», а сценарий Музыки — «не могу играть музыку на этом устройстве».

* Версию бэкенда сценария, для более удобного анализа логов запросов, в свойстве `Version`.

  В качестве версии удобно использовать, например, номер ревизии кода, который отвечает на запрос.

* Тело ответа, которое зависит от результата обработки реплики пользователя:

  * Сценарий может быстро и полностью обработать реплику (дальнейшей обработки не потребуется — например, возвращается прогноз погоды).

    Ответ должен включать свойство `ResponseBody` с телом ответа, который сразу после ранжирования можно отправить пользователю.

  * Сценарий может быстро вернуть готовый ответ, но ждет результата ранжирования, чтобы понять, нужно ли на самом деле менять состояние релевантных объектов (например, включить лампочку).

    Ответ должен включать свойство `CommitCandidate` с телом ответа пользователю и аргументами дальнейшего вызова ручки `/commit`.

  * Сценарию требуется значительное время, чтобы ответить (например, найти и подготовить музыкальный трек).

    Ответ должен включать свойство `ContinueArguments`, с произвольными данными для последующего вызова ручки `/continue`.

  * Сценарий не может разделить формулировку окончательного ответа пользователю и сайд-эффект (например, не получается ответить на запрос вызова такси, не вызвав такси).

    Ответ должен включать свойство `ApplyArguments`, с произвольными данными для последующего вызова ручки `/apply`.

  * При обработке реплики возникла ошибка.

    Сценарий должен вернуть свойство `Error` — произвольный текст и тип ошибки, которые потом можно использовать для анализа. Пользователь этот текст не увидит. Алиса ответит общей фразой о том, что что-то не работает.

    Этот вариант должен использоваться именно при неожиданном
    поведении сценария, он эквивалентен случаю когда сценарий вообще не
    ответил (таймаут, другие сетевые проблемы, http 500 и т.д.), но
    гораздо более предпочитительный для дальнейшей отладки и дебага.


### Ручка без сайд-эффекта с ответом после ранжирования (/continue) {#continue}

Если ответ ручки `/run` победил в ранжировании, и сценарий передал аргументы `ContinueArguments`, Мегамайнд вызовет ручку `/continue`.

В ответ сценарий должен вернуть:

* Готовый ответ пользователю в поле `ResponseBody`.
* Версию сценария в свойстве `Version`.

Ответ описывается элементом [TScenarioContinueResponse](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/scenarios/response.proto).

### Ручка фиксации изменений (/commit) {#commit}

Ручка `/commit` вызывается, чтобы произвести изменения, если ответ сценария на ручку `/run` победил в ранжировании. Ответ описывается элементом [TScenarioCommitResponse](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/scenarios/response.proto):

* Один из элементов `Success` или `Error` — сценарий должен сообщить, удалось ли сделать то, о чем просил пользователь.
* Версия сценария в свойстве `Version`.

Ручка `/commit` должна быть идемпотентной: при повторном запросе с тем же идентификатором `RequestId` запрошенная пользователем операция не должна запускаться повторно, а ответ должен быть таким же, как и в первый раз.

В идеале ручку `/commit` можно вызывать асинхронно: Мегамайнд не дожидается ответа `/commit`, чтобы отправить пользователю победивший в ранжировании ответ соответствующей ручки `/run`.


### Ручка ответа после сайд-эффекта (/apply) {#apply}

Если ответ ручки `/run` победил в ранжировании, и сценарий передал аргументы `ApplyArguments`, Мегамайнд вызовет ручку `/apply`.

Ручка `/apply` должна быть идемпотентной: при повторном запросе с тем же идентификатором `RequestId` запрошенная пользователем операция не должна запускаться повторно, а ответ должен быть таким же, как и в первый раз.

В ответе сценарий должен вернуть:

* Готовый ответ пользователю в поле `ResponseBody`.
* Версию сценария в свойстве `Version`.

Ответ описывается элементом [TScenarioApplyResponse](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/scenarios/response.proto).


## Примеры сессий запросов и ответов {#examples}

### Запрос POI с использованием NluHint и Callback-директив {#run-example}

В примере — два запроса и два ответа одной и той же ручки `/run`.

Диалог:

1. Пользователь говорит «кафе пушкин», сценарий POI получает [запрос](#run-request).
1. Сценарий [отвечает репликой с названием и адресом кафе](#run-response-hint). В ответ добавляется:
   * элемент `NluHint`, в котором перечислены подсказки о том, какие следующие реплики можно ожидать от пользователя после этого ответа;
   * элементы `Callback` с callback-директивами, которые нужно вызывать при определенных действиях пользователя.
1. Пользователь [спрашивает](#additional-run-request) о номере телефона, и сценарий получает callback-директиву для запроса информации с определенного URL.
1. Сценарий [отвечает](#additional-run-response) карточкой с номером телефона того же кафе «Пушкин».


#### 1. Запрос с названием заведения {#run-request}

Запрос пользователя — в поле `Input.Text.RawUtterance`.

```yaml
BaseRequest {
  RequestId: "95EB38D6-FA1C-4835-8B21-0023522AF013"
  ServerTimeMs: 1592561538941
  RandomSeed: 15838288000971308028
  ClientInfo {
    AppId: "ru.yandex.mobile.inhouse"
    AppVersion: "2400"
    OsVersion: "13.3.1"
    ...
  }
  Location {
    Lat: 55.788055419921882
    Lon: 37.672678070102947
    ...
  }
  Interfaces {
    HasScreen: true
    HasReliableSpeakers: true
    HasBluetooth: true
    HasMicrophone: true
  }
  DeviceState {
  }
  State {
  }
  Experiments {
    fields {
      key: "activation_search_redirect_experiment"
      value {
        string_value: "1"
      }
    }
    fields {
      key: "afisha_poi_events"
      value {
        string_value: "1"
      }
    }
  }
  Options {
    UserAgent: ...
    FiltrationLevel: 1
    ClientIP: "109.252.86.235"
    ScreenScaleFactor: 3
    RawPersonalData: ...
    UserDefinedRegionId: 213
  }
  IsNewSession: true
  UserPreferences {
  }
  IsSessionReset: true
  UserLanguage: L_RUS
}
Input {
  Text {
    RawUtterance: "Кафе пушкин"
    Utterance: "кафе пушкин"
  }
}

```

#### 2. Ответ о кафе с предполагаемыми вариантами уточнений (NluHint) {#run-response-hint}

Карточка, которую увидит пользователь, — в элементе `Layout`.
Варианты callback-директив — в элементах `FrameActions`.

```yaml
Features {
}
ResponseBody {
  Layout {
    Cards {
      Text: "Я знаю, что по адресу Тверской бул., 26А, Москва есть «Пушкинъ». Работает круглосуточно. "
    }
    OutputSpeech: "Я знаю, что по адресу Тверской бульвар, 26А, Москва есть «Пушкинъ». Работает круглосуточно. "
  }
  SemanticFrame {
    Name: "find_poi"
    Slots {
      Name: "what"
      Type: "string"
      Value: "what"
      AcceptedTypes: "string"
    }
  }
  State {
  }
  AnalyticsInfo {
    Intent: "personal_assistant.scenarios.find_poi"
  }
  FrameActions {
    key: "addr"
    value {
      NluHint {
        FrameName: "addr"
        Instances {
          Language: L_RUS
          Phrase: "какой адрес"
        }
        Instances {
          Language: L_RUS
          Phrase: "где находится"
        }
      }
      Callback {
        Name: "request_url"
        Payload {
          fields {
            key: "url"
            value {
              string_value: "https://yandex.ru/search/result?tmplrwr=web4%3Agoodwin&type=geov&mode=oid&ajax=%7B%22type%22%3A%22companies%22%2C%22subtype%22%3A%22company%22%7D&oid=b%3A1018907821&text=%D0%9F%D1%83%D1%88%D0%BA%D0%B8%D0%BD%D1%8A&action=addr&reqinfo=megamind.yandex.ru.yandex.mobile.inhouse&exp_flags=enable_goodwin_all_device%3BGEO_1org_dialog%3BGEO_enable_app"
            }
          }
        }
      }
    }
  }
  FrameActions {
    key: "hours"
    value {
      NluHint {
        FrameName: "hours"
        Instances {
          Language: L_RUS
          Phrase: "когда открыто"
        }
        Instances {
          Language: L_RUS
          Phrase: "часы работы"
        }
        Instances {
          Language: L_RUS
          Phrase: "режим работы"
        }
        Instances {
          Language: L_RUS
          Phrase: "время работы"
        }
      }
      Callback {
        Name: "request_url"
        Payload {
          fields {
            key: "url"
            value {
              string_value: "https://yandex.ru/search/result?tmplrwr=web4%3Agoodwin&type=geov&mode=oid&ajax=%7B%22type%22%3A%22companies%22%2C%22subtype%22%3A%22company%22%7D&oid=b%3A1018907821&text=%D0%9F%D1%83%D1%88%D0%BA%D0%B8%D0%BD%D1%8A&action=hours&reqinfo=megamind.yandex.ru.yandex.mobile.inhouse&exp_flags=enable_goodwin_all_device%3BGEO_1org_dialog%3BGEO_enable_app"
            }
          }
        }
      }
    }
  }
  FrameActions {
    key: "metro"
    value {
      NluHint {
        FrameName: "metro"
        Instances {
          Language: L_RUS
          Phrase: "станция метро"
        }
        Instances {
          Language: L_RUS
          Phrase: "скажи метро"
        }
        Instances {
          Language: L_RUS
          Phrase: "какое метро"
        }
      }
      Callback {
        Name: "request_url"
        Payload {
          fields {
            key: "url"
            value {
              string_value: "https://yandex.ru/search/result?tmplrwr=web4%3Agoodwin&type=geov&mode=oid&ajax=%7B%22type%22%3A%22companies%22%2C%22subtype%22%3A%22company%22%7D&oid=b%3A1018907821&text=%D0%9F%D1%83%D1%88%D0%BA%D0%B8%D0%BD%D1%8A&action=metro&reqinfo=megamind.yandex.ru.yandex.mobile.inhouse&exp_flags=enable_goodwin_all_device%3BGEO_1org_dialog%3BGEO_enable_app"
            }
          }
        }
      }
    }
  }
  FrameActions {
    key: "phone"
    value {
      NluHint {
        FrameName: "phone"
        Instances {
          Language: L_RUS
          Phrase: "какой номер телефона"
        }
        Instances {
          Language: L_RUS
          Phrase: "какой телефон"
        }
        Instances {
          Language: L_RUS
          Phrase: "скажи номер телефона"
        }
        Instances {
          Language: L_RUS
          Phrase: "куда звонить"
        }
      }
      Callback {
        Name: "request_url"
        Payload {
          fields {
            key: "url"
            value {
              string_value: "https://yandex.ru/search/result?tmplrwr=web4%3Agoodwin&type=geov&mode=oid&ajax=%7B%22type%22%3A%22companies%22%2C%22subtype%22%3A%22company%22%7D&oid=b%3A1018907821&text=%D0%9F%D1%83%D1%88%D0%BA%D0%B8%D0%BD%D1%8A&action=phone&reqinfo=megamind.yandex.ru.yandex.mobile.inhouse&exp_flags=enable_goodwin_all_device%3BGEO_1org_dialog%3BGEO_enable_app"
            }
          }
        }
      }
    }
  }
  FrameActions {
    key: "review"
    value {
      NluHint {
        FrameName: "review"
        Instances {
          Language: L_RUS
          Phrase: "какие отзывы"
        }
        Instances {
          Language: L_RUS
          Phrase: "скажи рейтинг"
        }
        Instances {
          Language: L_RUS
          Phrase: "какой рейтинг"
        }
      }
      Callback {
        Name: "request_url"
        Payload {
          fields {
            key: "url"
            value {
              string_value: "https://yandex.ru/search/result?tmplrwr=web4%3Agoodwin&type=geov&mode=oid&ajax=%7B%22type%22%3A%22companies%22%2C%22subtype%22%3A%22company%22%7D&oid=b%3A1018907821&text=%D0%9F%D1%83%D1%88%D0%BA%D0%B8%D0%BD%D1%8A&action=review&reqinfo=megamind.yandex.ru.yandex.mobile.inhouse&exp_flags=enable_goodwin_all_device%3BGEO_1org_dialog%3BGEO_enable_app"
            }
          }
        }
      }
    }
  }
}
Version: "0"
```

#### 3. Дополнительный вопрос про номер телефона {#additional-run-request}

Сработавший callback — в элементе `Input.Callback`.

```yaml
BaseRequest {
  RequestId: "6C5C5CF7-3375-4E05-A3A5-A81D517979E6"
  ServerTimeMs: 1592561652821
  RandomSeed: 2373567294183839854
  ClientInfo {
    AppId: "ru.yandex.mobile.inhouse"
    AppVersion: "2400"
    OsVersion: "13.3.1"
    ...
  }
  Location {
    Lat: 55.788055419921882
    Lon: 37.672680778087717
    ...
  }
  Interfaces {
    HasScreen: true
    HasReliableSpeakers: true
    HasBluetooth: true
    HasMicrophone: true
  }
  DeviceState {
  }
  State {
  }
  Experiments {
    fields {
      key: "activation_search_redirect_experiment"
      value {
        string_value: "1"
      }
    }
    fields {
      key: "afisha_poi_events"
      value {
        string_value: "1"
      }
    }
  }
  Options {
    UserAgent: "Mozilla/5.0 (iPhone; CPU iPhone OS 13_3_1 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Mobile/15E148 YaBrowser/19.5.2.38.10 YaApp_iOS/24.00 Safari/604.1"
    FiltrationLevel: 1
    ClientIP: "109.252.86.235"
    ScreenScaleFactor: 3
    RawPersonalData: "..."
    UserDefinedRegionId: 213
  }
  UserPreferences {
  }
  UserLanguage: L_RUS
}
Input {
  Callback {
    Name: "request_url"
    Payload {
      fields {
        key: "url"
        value {
          string_value: "https://yandex.ru/search/result?tmplrwr=web4%3Agoodwin&type=geov&mode=oid&ajax=%7B%22type%22%3A%22companies%22%2C%22subtype%22%3A%22company%22%7D&oid=b%3A1018907821&text=%D0%9F%D1%83%D1%88%D0%BA%D0%B8%D0%BD%D1%8A&action=phone&reqinfo=megamind.yandex.ru.yandex.mobile.inhouse&exp_flags=enable_goodwin_all_device%3BGEO_1org_dialog%3BGEO_enable_app"
        }
      }
    }
  }
}

```

#### 4. Ответ с номером телефона кафе из предыдущего ответа {#additional-run-response}

Запрошенный номер телефона — в карточке в элементе `Layout`.

```yaml
Features {
}
ResponseBody {
  Layout {
    Cards {
      Text: "Номер телефона «Пушкинъ» +7 (495) 739-00-33. "
    }
    OutputSpeech: "Номер телефона «Пушкинъ» +7 (495) 739-00-33. "
  }
  SemanticFrame {
    Name: "find_poi"
    Slots {
      Name: "what"
      Type: "string"
      Value: "what"
      AcceptedTypes: "string"
    }
  }
  State {
  }
  AnalyticsInfo {
    Intent: "personal_assistant.scenarios.find_poi"
  }
  FrameActions {
    key: "addr"
    value {
      NluHint {
        FrameName: "addr"
        Instances {
          Language: L_RUS
          Phrase: "какой адрес"
        }
        Instances {
          Language: L_RUS
          Phrase: "где находится"
        }
      }
      Callback {
        Name: "request_url"
        Payload {
          fields {
            key: "url"
            value {
              string_value: "https://yandex.ru/search/result?tmplrwr=web4%3Agoodwin&type=geov&mode=oid&ajax=%7B%22type%22%3A%22companies%22%2C%22subtype%22%3A%22company%22%7D&oid=b%3A1018907821&text=%D0%9F%D1%83%D1%88%D0%BA%D0%B8%D0%BD%D1%8A&action=addr&reqinfo=megamind.yandex.ru.yandex.mobile.inhouse&exp_flags=enable_goodwin_all_device%3BGEO_1org_dialog%3BGEO_enable_app"
            }
          }
        }
      }
    }
  }
  FrameActions {
    key: "hours"
    value {
      NluHint {
        FrameName: "hours"
        Instances {
          Language: L_RUS
          Phrase: "когда открыто"
        }
        Instances {
          Language: L_RUS
          Phrase: "часы работы"
        }
        Instances {
          Language: L_RUS
          Phrase: "режим работы"
        }
        Instances {
          Language: L_RUS
          Phrase: "время работы"
        }
      }
      Callback {
        Name: "request_url"
        Payload {
          fields {
            key: "url"
            value {
              string_value: "https://yandex.ru/search/result?tmplrwr=web4%3Agoodwin&type=geov&mode=oid&ajax=%7B%22type%22%3A%22companies%22%2C%22subtype%22%3A%22company%22%7D&oid=b%3A1018907821&text=%D0%9F%D1%83%D1%88%D0%BA%D0%B8%D0%BD%D1%8A&action=hours&reqinfo=megamind.yandex.ru.yandex.mobile.inhouse&exp_flags=enable_goodwin_all_device%3BGEO_1org_dialog%3BGEO_enable_app"
            }
          }
        }
      }
    }
  }
  FrameActions {
    key: "metro"
    value {
      NluHint {
        FrameName: "metro"
        Instances {
          Language: L_RUS
          Phrase: "станция метро"
        }
        Instances {
          Language: L_RUS
          Phrase: "скажи метро"
        }
        Instances {
          Language: L_RUS
          Phrase: "какое метро"
        }
      }
      Callback {
        Name: "request_url"
        Payload {
          fields {
            key: "url"
            value {
              string_value: "https://yandex.ru/search/result?tmplrwr=web4%3Agoodwin&type=geov&mode=oid&ajax=%7B%22type%22%3A%22companies%22%2C%22subtype%22%3A%22company%22%7D&oid=b%3A1018907821&text=%D0%9F%D1%83%D1%88%D0%BA%D0%B8%D0%BD%D1%8A&action=metro&reqinfo=megamind.yandex.ru.yandex.mobile.inhouse&exp_flags=enable_goodwin_all_device%3BGEO_1org_dialog%3BGEO_enable_app"
            }
          }
        }
      }
    }
  }
  FrameActions {
    key: "phone"
    value {
      NluHint {
        FrameName: "phone"
        Instances {
          Language: L_RUS
          Phrase: "какой номер телефона"
        }
        Instances {
          Language: L_RUS
          Phrase: "какой телефон"
        }
        Instances {
          Language: L_RUS
          Phrase: "скажи номер телефона"
        }
        Instances {
          Language: L_RUS
          Phrase: "куда звонить"
        }
      }
      Callback {
        Name: "request_url"
        Payload {
          fields {
            key: "url"
            value {
              string_value: "https://yandex.ru/search/result?tmplrwr=web4%3Agoodwin&type=geov&mode=oid&ajax=%7B%22type%22%3A%22companies%22%2C%22subtype%22%3A%22company%22%7D&oid=b%3A1018907821&text=%D0%9F%D1%83%D1%88%D0%BA%D0%B8%D0%BD%D1%8A&action=phone&reqinfo=megamind.yandex.ru.yandex.mobile.inhouse&exp_flags=enable_goodwin_all_device%3BGEO_1org_dialog%3BGEO_enable_app"
            }
          }
        }
      }
    }
  }
  FrameActions {
    key: "review"
    value {
      NluHint {
        FrameName: "review"
        Instances {
          Language: L_RUS
          Phrase: "какие отзывы"
        }
        Instances {
          Language: L_RUS
          Phrase: "скажи рейтинг"
        }
        Instances {
          Language: L_RUS
          Phrase: "какой рейтинг"
        }
      }
      Callback {
        Name: "request_url"
        Payload {
          fields {
            key: "url"
            value {
              string_value: "https://yandex.ru/search/result?tmplrwr=web4%3Agoodwin&type=geov&mode=oid&ajax=%7B%22type%22%3A%22companies%22%2C%22subtype%22%3A%22company%22%7D&oid=b%3A1018907821&text=%D0%9F%D1%83%D1%88%D0%BA%D0%B8%D0%BD%D1%8A&action=review&reqinfo=megamind.yandex.ru.yandex.mobile.inhouse&exp_flags=enable_goodwin_all_device%3BGEO_1org_dialog%3BGEO_enable_app"
            }
          }
        }
      }
    }
  }
}
Version: "7534756"

```


### Запрос музыки (/run + /continue) {#continue}

Диалог:

1. Пользователь говорит: «Включи death metal» (`Text.Utterance`), реплика матчится с несколькими фреймами, на которые подписан сценарий Музыки. Алиса спрашивает у сценария Музыки, что бы на это ответить.
1. Музыка отвечает описанием плейлиста с треками нужного жанра.
1. Алиса отправляет запрос на проигрывание плейлиста.
1. Музыка отвечает нужными данными.

В итоге Алиса говорит пользователю «Включаю подборку» и выполняет директиву `MusicPlayDirective`, которая запускает воспроизведение.

#### 1. Запрос «включи death metal» к ручке /run {#continue-run-request}

Реплика пользователя (`Text`) и соответствующие ей фреймы (`SemanticFrames`) — в элементе `Input`.

```yaml
BaseRequest {
  RequestId: "d2f4d17f-d406-4c09-af25-1a119293ad90"
  ServerTimeMs: 1595319199000
  RandomSeed: 7060042687719557867
  ClientInfo {
    AppId: "ru.yandex.quasar.testbot"
    AppVersion: "10"
    OsVersion: "8.1.0"
    ...
  }
  Location {
    Lat: 55.733771
    Lon: 37.587937
    ...
  }
  Interfaces {
    HasBluetooth: true
    HasMicrophone: true
    ...
  }
  DeviceState {
    SoundLevel: 5
    SoundMuted: true
    ...
  }
  State {
  }
  Experiments {
    fields {
      key: "mm_dont_defer_apply"
      value {
        string_value: "1"
      }
    }
    fields {
      key: "mm_enable_protocol_scenario=Miles"
      value {
        string_value: ""
      }
    }
  }
  Options {
    FiltrationLevel: 1
    ClientIP: "0:0:0:0:0:0:0:0"
    UserDefinedRegionId: 1
  }
  UserPreferences {
  }
  UserLanguage: L_RUS
}
Input {
  SemanticFrames {
    Name: "personal_assistant.scenarios.music_play"
    Slots {
      Name: "action_request"
      Type: "action_request"
      Value: "autoplay"
      AcceptedTypes: "action_request"
    }
    Slots {
      Name: "search_text"
      Type: "string"
      Value: "death metal"
      AcceptedTypes: "string"
    }
  }
  Text {
    RawUtterance: "включи death metal"
    Utterance: "включи death metal"
  }
}
```

#### 2. Ответ /run, с фичами Музыки и аргументами для вызова /continue {#continue-run-response}

Фичи для ранжирования — в элементе `Features`, аргументы для последующего вызова `/continue` - в элементе `ContinueArguments`.

```yaml
Features {
  MusicFeatures {
    Result {
      TrackNameSimilarity {
        QueryInResponse {
          Max: 1
          Mean: 1
          Min: 1
        }
        ...
    }
    Wizard {
      TitleSimilarity {
        QueryInResponse {
          Max: 1
          Mean: 1
          Min: 1
        }
        ...
      }
    }
    ...
  }
}
Version: "hollywood/stable-22-4@7111630"
ContinueArguments {
  type_url: "type.googleapis.com/NAlice.NHollywood.TMusicArguments"
  ...
}
```

#### 3. Запрос к ручке /continue с аргументами из предыдущего ответа /run

Элемент `Arguments` содержит аргументы, которые вернул соответствующий ответ ручки `/run`.

```yaml
Arguments { # содержимое ContinueArguments из ответа ручки /run
  type_url: "type.googleapis.com/NAlice.NHollywood.TMusicArguments"
  ...
}
BaseRequest {
  RequestId: "d2f4d17f-d406-4c09-af25-1a119293ad90"
  ServerTimeMs: 1595319199000
  RandomSeed: 7060042687719557867
  ClientInfo {
    AppId: "ru.yandex.quasar.testbot"
    AppVersion: "10"
    OsVersion: "8.1.0"
    ...
  }
  Location {
    Lat: 55.733771
    Lon: 37.587937
    ...
  }
  Interfaces {
    HasBluetooth: true
    HasMicrophone: true
    ...
  }
  DeviceState {
    SoundLevel: 5
    SoundMuted: true
    ...
  }
  State {
  }
  Experiments {
    fields {
      key: "mm_enable_protocol_scenario=Miles"
      value {
        string_value: ""
      }
    }
    fields {
      key: "music_no_enqueue"
      value {
        string_value: ""
      }
    }
  }
  Options {
    FiltrationLevel: 1
    ClientIP: "0:0:0:0:0:0:0:0"
    UserDefinedRegionId: 1
  }
  UserPreferences {
  }
  UserLanguage: L_RUS
}
Input {
  SemanticFrames {
    Name: "personal_assistant.scenarios.music_play"
    Slots {
      Name: "action_request"
      Type: "action_request"
      Value: "autoplay"
      AcceptedTypes: "action_request"
    }
    Slots {
      Name: "search_text"
      Type: "string"
      Value: "death metal"
      AcceptedTypes: "string"
    }
  }
  Text {
    RawUtterance: "включи death metal"
    Utterance: "включи death metal"
  }
}
```

#### 4. Ответ ручки /continue, включающий музыку

С директивой `MusicPlayDirective`, которая запускает на клиенте первый трек из найденного плейлиста.

```yaml
ResponseBody {
  Layout {
    Cards {
      Text: "Включаю подборку "
      Death metal"."
    }
    OutputSpeech: "Включаю подборку \"Death metal\""
    Directives {
      MusicPlayDirective {
        Name: "music_smart_speaker_play"
        SessionId: "J6M5H8A9"
        FirstTrackId: "32226837"
      }
    }
  }
  SemanticFrame {
    Name: "personal_assistant.scenarios.music_play"
    Slots {
      Name: "action_request"
      Type: "action_request"
      Value: "autoplay"
      AcceptedTypes: "action_request"
    }
    Slots {
      Name: "answer"
      Type: "music_result"
      AcceptedTypes: "music_result"
    }
    Slots {
      Name: "search_text"
      Type: "string"
      Value: "death metal"
      AcceptedTypes: "string"
    }
  }
  AnalyticsInfo {
    Intent: "personal_assistant.scenarios.music_play"
    Objects {
      Id: "music.first_track_id"
      Name: "first_track_id"
      HumanReadable: "Benighted, Reptilian"
      FirstTrack {
        Id: "32226837"
        Genre: "extrememetal"
        Duration: "196790"
      }
    }
    ProductScenarioName: "music"
    ...
  }
}
Version: "hollywood/stable-22-4@7111630"
```


### Запрос платного видео (/run + /commit) {#commit}

Диалог:

1. Пользователь просит включить платное кино на Кинопоиске. [Запрос](#commit-run-request) уходит в сценарий Видео.
1. В [ответе сценария](#commit-run-response) — предложение заплатить за кино в приложении.
1. Когда сценарий побеждает в ранжировании, Мегамайнд отправляет в ручку `/commit` [запрос на генерацию пуша для оплаты](#commit-commit-request).
1. Сценарий [отвечает](#commit-commit-response), что серверная логика отработала нормально.

В итоге Алиса говорит пользователю «Откройте приложение Яндекса для легкой и быстрой оплаты. Прислала вам ссылку в телефон.», а от Кинопоиска приходит пуш для оплаты.

#### 1. Запрос платного видео (/run) {#commit-run-request}

Реплика пользователя — в элементе `Voice`.

```yaml
BaseRequest {
  RequestId: "5ba48043-096d-45da-9cfe-97ce4d996142"
  ServerTimeMs: 1595354108574
  RandomSeed: 16290130458729062562
  ClientInfo {
    AppId: "ru.yandex.quasar.app"
    AppVersion: "1.0"
    OsVersion: "6.0.1"
    ...
  }
  Location {
    Lat: 56.004863739013672
    Lon: 37.445953369140618
    ...
  }
  Interfaces {
    VoiceSession: true
    HasReliableSpeakers: true
    HasBluetooth: true
    ...
  }
  DeviceState {
  }
  State {
  }
  Experiments {
    fields {
      key: "alarm_how_long"
      value {
        string_value: "1"
      }
    }
    fields {
      key: "alarm_snooze"
      value {
        string_value: "1"
      }
    }
    ...
  }
  Options {
    UserAgent: "Mozilla\\/5.0 (Linux; Android 6.0.1; Station Build\\/MOB30J; wv) AppleWebKit\\/537.36 (KHTML, like Gecko) Version\\/4.0 Chrome\\/61.0.3163.98 Safari\\/537.36 YandexStation\\/2.3.0.3.373060213.20190204.develop.ENG (YandexIO)"
    ...
  }
  UserPreferences {
  }
  UserLanguage: L_RUS
  Input {
    SemanticFrames {
      Name: "personal_assistant.scenarios.quasar.open_current_video"
      Slots {
        Name: "action"
        Type: "custom.video_selection_action"
        Value: "play"
        AcceptedTypes: "custom.video_selection_action"
      }
    }
    SemanticFrames {
      Name: "personal_assistant.scenarios.video_play_text"
      Slots {
        Name: "action"
        Type: "string"
        Value: "включи"
        AcceptedTypes: "string"
      }
    }
    Voice {
      Utterance: "включи" # реплика пользователя
      AsrData {
        Utterance: "включи."
        Confidence: 0
        Words {
          Value: "включи"
          Confidence: 1
        }
      }
      BiometryScoring {
        Status: "ok"
        RequestId: "18af9c20-4630-4f4d-90b1-8da0aeb7469d"
        GroupId: "1454ee3687208ea4ee2ee12e944947c4"
      }
      BiometryClassification {
        ...
      }
    }
  }
  DataSources {
    ...
  }
}

```

#### 2. Ответ /run с репликой Алисы и аргументами для /commit {#commit-run-response}

Ответ сценария, который следует отправить пользователю при победе в ранжировании, — в элементе `CommitCandidate`. В элементе `Arguments` — аргументы для последующего вызова `/commit`.

```yaml
Features {
  VideoFeatures {
    IsSearchVideo: 0
    IsSelectVideoFromGallery: 0
    IsPaymentConfirmed: 0
    IsAuthorizeProvider: 0
    IsOpenCurrentVideo: 1
    IsGoToVideoScreen: 0
    ItemSelectorConfidence: 1
  }
  Intent: "mm.personal_assistant.scenarios.quasar.open_current_video"
}
CommitCandidate {
  ResponseBody {
    Layout {
      Cards {
        Text: "Откройте приложение Яндекса для легкой и быстрой оплаты. Прислала вам ссылку в телефон."
      }
      OutputSpeech: "Откройте приложение Яндекса для легкой и быстрой оплаты. Прислала вам ссылку в телефон."
      Directives {
        ShowPayPushScreenDirective {
          Name: "video_show_pay_push_screen"
          Item {
            Type: "movie"
            ProviderName: "kinopoisk"
            ProviderItemId: "496a6a063cc043a194528e8dd80cfad6"
            MiscIds {
              Kinopoisk: "762738"
            }
            ...
          }
        }
      }
    }
    AnalyticsInfo {
      Intent: "mm.personal_assistant.scenarios.quasar.open_current_video"
    }
  }
  Arguments {
    type_url: "type.googleapis.com/google.protobuf.StringValue"
    ...
  }
}
Version: "@7134170"
```


#### 3. Запрос на генерацию пуша оплаты (/commit) {#commit-commit-request}

Элемент `Arguments` содержит аргументы из предыдущего ответа ручки `/run`.

```yaml
Arguments { # содержимое Arguments из ответа ручки /run
  type_url: "type.googleapis.com/google.protobuf.StringValue"
  ...
}
BaseRequest {
  RequestId: "5ba48043-096d-45da-9cfe-97ce4d996142"
  ServerTimeMs: 1595354110011
  RandomSeed: 16290130458729062562
  ClientInfo {
    AppId: "ru.yandex.quasar.app"
    AppVersion: "1.0"
    OsVersion: "6.0.1"
    ...
  }
  Location {
    Lat: 56.004863739013672
    Lon: 37.445953369140618
    ...
  }
  Interfaces {
    VoiceSession: true
    HasReliableSpeakers: true
    HasBluetooth: true
    ...
  }
  DeviceState {
    DeviceId: "74005034440c082106ce"
    SoundLevel: 4
    SoundMuted: false
    IsTvPluggedIn: true
    Music { ... }
    Video { ... }
  }
  State {
  }
  Experiments {
    fields {
      key: "analytics_info"
      value {
        string_value: "1"
      }
    }
    fields {
      key: "bg_enable_get_news_experimental"
      value {
        string_value: "1"
      }
    }
  }
  Options {
    UserAgent: "Mozilla\\/5.0 (Linux; Android 6.0.1; Station Build\\/MOB30J; wv) AppleWebKit\\/537.36 (KHTML, like Gecko) Version\\/4.0 Chrome\\/61.0.3163.98 Safari\\/537.36 YandexStation\\/2.3.0.3.373060213.20190204.develop.ENG (YandexIO)"
    ...
  }
  UserPreferences {
  }
  UserLanguage: L_RUS
  }
  Input {
    Voice {
      Utterance: "включи"
      AsrData {
        Utterance: "включи."
        Confidence: 0
        Words {
          Value: "включи"
          Confidence: 1
        }
      }
    BiometryScoring { ... }
    BiometryClassification { ... }
  }
}
```

#### 4. Результат (/commit) {#commit-commit-response}

В ответе `/commit` — только сообщение об успехе или ошибке серверного сайд-эффекта.

```yaml
Success {
}
Version: "@7134170"
```

### Запрос платного видео (/run + /apply) {#apply}

Диалог:

1. Пользователь просит включить платное кино на Кинопоиске. [Запрос](#apply-run-request) уходит в сценарий Видео.
1. В [ответе](#apply-run-response) сценария — фичи, которые говорят о том, что выбран платный видеоконтент, и аргументы для генерации пуша в ручке `/apply`.
1. Когда сценарий побеждает в ранжировании, Мегамайнд отправляет в ручку `/apply` [запрос на генерацию пуша для оплаты](#apply-apply-request).
1. Сценарий [отвечает](#apply-apply-response) репликой «Откройте приложение Яндекса для легкой и быстрой оплаты. Прислала вам ссылку в телефон.» и генерирует пуш для оплаты.

#### 1. Запрос платного видео (/run) {#apply-run-request}

В элементе `Input` — элемент `Voice` с репликой пользователя и элементы `SemanticFrames` с фреймами, которые Бегемот сопоставил с этой репликой.

```yaml
BaseRequest {
  RequestId: "b14e361d-82d5-4fd5-91ba-ba815b019ec3"
  ServerTimeMs: 1595354392289
  RandomSeed: 18083752849044566506
  ClientInfo {
    AppId: "ru.yandex.quasar.app"
    AppVersion: "1.0"
    OsVersion: "6.0.1"
    ...
  }
  Location {
    Lat: 56.004863739013672
    Lon: 37.445953369140618
    ...
  }
  Interfaces {
    VoiceSession: true
    HasReliableSpeakers: true
    HasBluetooth: true
    ...
  }
  DeviceState {
    DeviceId: "74005034440c082106ce"
    SoundLevel: 4
    SoundMuted: false
    IsTvPluggedIn: true
    Music { ...
    }
    Video { ...
    }
    ...
  }
  State {
  }
  Experiments {
    fields {
      key: "alarm_how_long"
      value {
        string_value: "1"
      }
    }
    ...
  }
  Options {
    UserAgent: "Mozilla\\/5.0 (Linux; Android 6.0.1; Station Build\\/MOB30J; wv) AppleWebKit\\/537.36 (KHTML, like Gecko) Version\\/4.0 Chrome\\/61.0.3163.98 Safari\\/537.36 YandexStation\\/2.3.0.3.373060213.20190204.develop.ENG (YandexIO)"
    ...
  }
  IsNewSession: true
  UserPreferences {
  }
  UserLanguage: L_RUS
}
Input {
  SemanticFrames {
    Name: "personal_assistant.scenarios.quasar.payment_confirmed"
  }
  SemanticFrames {
    Name: "personal_assistant.scenarios.quasar.select_video_from_gallery_by_text"
  }
  SemanticFrames {
    Name: "personal_assistant.scenarios.video_play_text"
    Slots {
      Name: "action"
      Type: "string"
      Value: "купи"
      AcceptedTypes: "string"
    }
  }
  SemanticFrames {
    Name: "personal_assistant.scenarios.video_play"
  }
  Voice {
    Utterance: "купи"
    AsrData {
      Utterance: "купи."
      Confidence: 0
      Words {
        Value: "купи"
        Confidence: 1
      }
    }
    BiometryScoring { ...
    }
    BiometryClassification { ...
    }
  }
}
DataSources {
    ...
}
```


#### 2. Ответ /run с фичами Видео {#apply-run-response}

Аргументы для последующего вызова `/apply` — в поле `ApplyArguments`.

```yaml
ApplyArguments {
  type_url: "type.googleapis.com/google.protobuf.StringValue"
  ...
}
Features {
  VideoFeatures {
    IsSearchVideo: 0
    IsSelectVideoFromGallery: 0
    IsPaymentConfirmed: 1
    IsAuthorizeProvider: 0
    IsOpenCurrentVideo: 0
    IsGoToVideoScreen: 0
    ItemSelectorConfidence: 1
  }
  Intent: "mm.personal_assistant.scenarios.quasar.payment_confirmed"
}
Version: "@7134170"
```


#### 3. Запрос генерации пуша для оплаты (/apply) {#apply-apply-request}

В элементе `Arguments` — аргументы из предыдущего ответа ручки `/run`.

```yaml
Arguments {
  type_url: "type.googleapis.com/google.protobuf.StringValue"
  ...
}
BaseRequest {
  RequestId: "b14e361d-82d5-4fd5-91ba-ba815b019ec3"
  ServerTimeMs: 1595354393173
  RandomSeed: 18083752849044566506
  ClientInfo {
    AppId: "ru.yandex.quasar.app"
    AppVersion: "1.0"
    OsVersion: "6.0.1"
    ...
  }
}
Location {
  Lat: 56.004863739013672
  Lon: 37.445953369140618
  ...
}
Interfaces {
  VoiceSession: true
  HasReliableSpeakers: true
  HasBluetooth: true
  ...
}
DeviceState {
  DeviceId: "74005034440c082106ce"
  SoundLevel: 4
  SoundMuted: false
  IsTvPluggedIn: true
  Music { ...
  }
  Video { ...
  }
  ...
}
State {
}
Experiments {
  fields {
    key: "alarm_how_long"
    value {
      string_value: "1"
    }
  }
  ...
}
Options {
  UserAgent: "Mozilla\\/5.0 (Linux; Android 6.0.1; Station Build\\/MOB30J; wv) AppleWebKit\\/537.36 (KHTML, like Gecko) Version\\/4.0 Chrome\\/61.0.3163.98 Safari\\/537.36 YandexStation\\/2.3.0.3.373060213.20190204.develop.ENG (YandexIO)"
  ...
}
UserPreferences {
}
UserLanguage: L_RUS
Input {
  Voice {
    Utterance: "купи"
    AsrData {
      Utterance: "купи."
      Confidence: 0
      Words {
        Value: "купи"
        Confidence: 1
      }
    }
    BiometryScoring { ... }
    BiometryClassification { ... }
  }
}

```


#### 4. Ответ Алисы (/apply) {#apply-apply-response}

```yaml
ResponseBody {
  Layout {
    Cards {
      Text: "Откройте приложение Яндекса для легкой и быстрой оплаты. Прислала вам ссылку в телефон."
    }
    OutputSpeech: "Откройте приложение Яндекса для легкой и быстрой оплаты. Прислала вам ссылку в телефон."
    Directives {
      ShowPayPushScreenDirective {
        Name: "video_show_pay_push_screen"
        Item {
          Type: "movie"
          ProviderName: "kinopoisk"
        }
      }
    }
  }
  AnalyticsInfo {
    Intent: "mm.personal_assistant.scenarios.quasar.payment_confirmed"
  }
}
Version: "@7134170"
```
