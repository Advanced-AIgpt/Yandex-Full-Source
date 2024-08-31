# Конфигурация сценария

Информация о том, какие сценарии должны быть доступны Алисе, хранится в [папке конфигов Мегамайнда](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/configs). Каждый конфиг содержит формализованную информацию о назначении и атрибутах сценария (имя, поддерживаемый язык, URL бэкенда и т. д.)


## Доступные окружения Мегамайнда {#environments}

Для каждого из доступных окружений нужно написать отдельный конфиг:

* [dev](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/configs/dev/scenarios) — окружение для локальной разработки и CI Мегамайнда.
* [production](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/configs/production/scenarios) — окружение для продакшен-версий сценариев.
* [hamster](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/configs/hamster/scenarios) — окружение для прокачки аналитических запросов. Обычно используются копии конфигов продакшен-версий (возможно, с параметрами бэкенда, адаптированными для Hamster).
* [rc](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/configs/rc/scenarios) — окружение для тестирования сценариев перед релизом. Обычно используются копии конфигов продакшен-версий (возможно, с параметрами бэкенда предназначенного для тестирования биллинга).


## Формат конфига {#format}

Рекомендуется заполнять каждое поле конфига:

```bash
Name: "WeatherRequest"

Languages: [ L_RUS, L_TUR ]

AcceptedFrames: [
    "alice.weather.whats_the_weather_like_now",
    "alice.weather.whats_the_weather_in_city"
]

AcceptsAnyUtterance: False

DataSources: [
    {
        Type: WEB_SEARCH_DOCS,
        IsRequired: False
    }
]

Handlers: {
    BaseUrl: "http://weather.n.yandex-team.ru/"
    Tvm2ClientId: "2000464"
    RequestType: AppHostPure
}

Enabled: False

Owners: ["abc:weather_service", "weather_man"]

Description: "Сценарий вопроса о погоде"

DescriptionUrl: "https://yandex.ru/pogoda"

AcceptsImageInput: False

AcceptsMusicInput: False
```

Поддерживаемые поля:

* `Name` — название сценария (в CamelCase, например, `WeatherRequest`). По этому названию будут кластеризоваться метрики на графиках аналитики Мегамайнда.

  Название файла конфига должно совпадать с названием сценария.

* `Languages` — языки, которые поддерживает сценарий. Список всех допустимых языков приведен в [спецификации](https://a.yandex-team.ru/arc/trunk/arcadia/alice/protos/data/language/language.proto).

  Выбирая подходящие сценарии, Мегамайнд отфильтровывает те, которые
  не поддерживают язык пользователя. Отвечать пользователю нужно на том же
  языке на котором пришел запрос (это указано в поле [UserLanguage](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/scenarios/request.proto?rev=7208662#L372))

* `AcceptedFrames` — фреймы, на которые подписан сценарий. Если при
  разборе реплики Бегемот найдет фрейм из указанного списка, то
  Мегамайнд сделает `/run` запрос в сценарий, а если не найдет, то
  сценарий будет исключен из дальнейшего ранжирования.

* `AcceptsAnyUtterance` — признак сценария, который можно вызывать независимо от разбора Бегемота.

  Допустимые значения:

  * `False` — сценарий следует вызывать только для фреймов, перечисленных в поле `AcceptedFrames`.
  * `True` — сценарий можно вызывать при любом результате разбора реплики в Бегемоте.

  Если для сценария не указан ни список фреймов `AcceptedFrames`, ни `AcceptsAnyUtterance: True`, конфиг не пройдет проверку — получится, что сценарий не нужно вызывать никогда.

  {% note info "" %}

  Сценарий со значением `AcceptsAnyUtterance: True` будет получать
  запросы с каждой репликой, которую произносят пользователи Алисы.
  Этот вариант не рекомендован командой Алисы, всегда лучше уметь
  фильтровать поток запросов в свой сценарий. Используя данную опцию
  хорошо рассчитывайте нагрузку на ваш бэкенд и бекенды которые стоят
  за вашим, потому что на Алису идет очень большой RPS запросов от пользователей.

  {% endnote %}

* `DataSources` — нужные сценарию [источники данных](../architecture.md#sources).
  * `Type` - тип источника данных. Список всех типов можно посмотреть
    [в аркадии](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/data_source_type.proto?rev=7185957#L8).

  * `IsRequired` - если источник помечен как `IsRequired: True`, то при неудаче или отсутствии этого источника, запрос в сценарий не будет осуществлен. Значение по умолчанию `False` можно опустить.

* `Handlers` - параметры вызова для бэкенда сценария в данном окружении.

  * `BaseUrl` — URL бэкенда сценария, обязательное поле. Мегамайнд может вызывать ручки `/run` и `/apply`, в примере — `http://weather.n.yandex-team.ru/run` и `http://weather.n.yandex-team.ru/apply` соответственно.

    Чтобы Мегамайнд мог обратиться к бэкенду, нужны [дырки](https://puncher.yandex-team.ru/) из макроса _GENCFG_BASSPRODNETS_ к указанному адресу.

  * `Tvm2ClientId` — идентификатор [TVM-приложения](https://wiki.yandex-team.ru/passport/tvm2/quickstart/) сценария, обязательное поле.

  * `RequestType` - тип сценария. Подробнее описано [протоколе работы](protocol#scenario-types).

* `Enabled` — признак активности сценария, сценарии с полем `Enabled: False` по умолчанию игнорируются Мегамайндом. До окончания разработки и отладки (включая A/B-тестирование) поле должно оставаться со значением `False`.

  Во всех окружениях значение флага `Enabled` должно быть одинаково для одного и того же сценария: нельзя выключить сценарий в production, но включить в dev.

  Если тесты проходят успешно, конфиги с полем `Enabled: False` можно мержить в `arc/trunk` самостоятельно, без дополнительных согласований. Если есть сомнения, призывайте в ревью [@zubchick](https://staff.yandex-team.ru/zubchick), [@alkapov](https://staff.yandex-team.ru/alkapov) или [@g-kostin](https://staff.yandex-team.ru/g-kostin).
  Пока в конфигурационном файле указано `Enabled: False` для проведения эксперимента и [отладки](../testing/scenario-app-testing.md#new-scenario) можно пользоваться флагом эксперимента `mm_enable_protocol_scenario=<Name>`, где `Name` это имя сценария из этого конфига.
  После того как сценарий выкатился в продакшен и флаг будет заменен на `Enabled: True` станет доступен флаг `mm_disable_protocol_scenario=<Name>`, где `Name` это имя сценария из этого конфига. Такой флаг нужен например для обратного АБ эксперимента.

* `Owners` — ответственные за сценарий. В списке обязательно должен быть хотя бы один ABC-сервис.

  Формат:

  * `"abc:myservice"` - слаг ABC-сервиса из адреса, `https://abc.yandex-team.ru/services/myservice`.
  * `"login"` — логин сотрудника.

* `Description` — описание сценария как продуктовой фичи Алисы. Если описания нет, или оно пустое, сборка Мегамайнда не пройдет.

* `DescriptionUrl` — релевантная ссылка на сервис или проект, к которому относится сценарий.

* `AcceptsImageInput` — сценарий может обработать изображение, полученное в запросе.

* `AcceptsMusicInput` — сценарий может обработать результат распознавания музыкальной композиции.
