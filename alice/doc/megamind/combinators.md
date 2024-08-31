# Комбинаторы

Комбинаторы дают возможность сформировать итоговый ответ Алисы из ответов нескольких обычных сценариев. Все комбинаторы, доступные Мегамайнду, описаны в [конфигах](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/configs/production/combinators).

Комбинаторы не классифицируются вместе со сценариями. Можно считать, что сценарный постклассификатор является комбинатором с наименьшим приоритетом.

## Как работает

Комбинатор, как и сценарий, может подписаться на активационный фрейм. Если этот фрейм в Мегамайнде считается активным (разобран бегемотом, пришел в запросе и т.д.), то после опроса всех сценариев в обычном режиме, происходит запрос в сам комбинатор, содержащий нужные ответы сценариев. Возможны три варианта ответа:
1. `Irrelevant` ответ – Мегамайнд выбирает победивший сценарий как обычно.
2. `Relevant` ответ – Мегамайнд выбирает победивший комбинатор.
3. `ContinueArguments` – Мегамайнд выбирает победивший комбинатор и запускается `CONTINUE` стадия комбинатора.

{% note info "" %}

Классификация комбинаторов не реализована. Сейчас выбирается любой из ответивших.

{% endnote %}

На стадии `CONTINUE` делается повторный запрос в комбинатор, который дополняется ответами сценариев на `CONTINUE` стадии. Мегамайнд ожидает `Relevant` ответ от комбинатора.

{% note info "" %}

`Apply` и `Commit` стадии не реализованы в комбинаторах.

{% endnote %}

## Конфиг комбинатора

Схему конфигов можно посмотреть [тут](https://a.yandex-team.ru/arcadia/alice/megamind/library/config/scenario_protos/combinator_config.proto).
Конфиги текущих комбинаторов лежат [тут](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/configs/production/combinators):

Поле | Описание
--- | ---
Name | название комбинатора
AcceptedFrames | активационные фреймы
AcceptsAllFrames | если True, запускается на каждый запрос
AcceptedScenarios | список сценариев, необходимых комбинатору
AcceptsAllScenarios | если True, комбинатор получает ответы всех сценариев, которые были запущены
Description | описание комбинатора
Enabled | включен/выключен
Dependences | описание DataSources необходимых комбинатору
Responsibles | ответственные за комбинатор

## DataSources

Если комбинатору нужны ответы каких-то источников данных, то нужно указать их в конфиге в поле `Dependences` в формате:

```yaml

Dependences: [
    {
        NodeName: "BLACKBOX_HTTP", // имя ноды в графе megamind (https://a.yandex-team.ru/arc/trunk/arcadia/apphost/conf/verticals/ALICE/megamind.json)
        Items: [
            {
                ItemName: "blackbox_http_response", // название айтема из ноды
                IsRequired: True // если True, то при отсутствии этого источника комбинатор не будет запущен
            }
        ]
    }
]

```

{% note info "" %}

Если нода с источником отсутствует в зависимостях `COMBINATORS_CONTINUE` и `COMBINATORS_RUN` в графе [megamind.json](https://a.yandex-team.ru/arc/trunk/arcadia/apphost/conf/verticals/ALICE/megamind.json), нужно добавить её вручную.

{% endnote %}

## Протокол

Мегамайнд отправляет комбинатору [TCombinatorRequest](https://a.yandex-team.ru/arcadia/alice/megamind/protos/scenarios/combinator_request.proto) и получает в ответе [TCombinatorResponse](https://a.yandex-team.ru/arcadia/alice/megamind/protos/scenarios/combinator_response.proto).
В запросе приходит только базовая информация `BaseRequest` и список фреймов, на которые сработал комбинатор в поле `Input`.

{% note info "" %}

Поля `ScenarioResponses` и `DataSources` являются `deprecated`. Ответы сценариев и источников приходят в комбинатор как отдельные аппхостовые айтемы.

{% endnote %}

В ответе комбинатор должен отдать:
1.  `Response` – может быть `Relevant`, `Irrelevant` или `ContinueArguments`. По факту является привычным [TScenarioRunResponse](https://a.yandex-team.ru/arcadia/alice/megamind/protos/scenarios/response.proto?rev=r9551140#L356).
1.  `UsedScenarios` для обновления стейта только использованых сценариев.
1.  `CombinatorsAnalyticsInfo`.

## Быстрый старт

1. Добавляем конфиг [сюда](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/configs/dev/combinators) (и сразу в hamster/rc/production с настройкой `Enabled=False`):

```yaml
Name: "MyCombinator"
AcceptsAllFrames: True
AcceptsAllScenarios: True
Description: "My first combinator"
Enabled: True
```

2. Добавляем граф комбинатора `combinator_my_combinator_run.json`. Если нужно, добавляем граф для `CONTINUE` стадии `combinator_my_combinator_continue.json`. Формат названия графов важен: `combinator_(snake_case_combinator_name)_(run/continue).json`. Пример:

```json
{
    "settings": {
        "input_deps": [
            "COMBINATORS_SETUP",
            "INPUT_SCENARIOS_RUN"
        ],
        "node_deps": {
            "MY_COMBINATOR": {
                "input_deps": [
                    "COMBINATORS_SETUP@!combinator_request_apphost_type",
                    "INPUT_SCENARIOS_RUN"
                ]
            },
            "RESPONSE": {
                "input_deps": [
                    "MY_COMBINATOR@!combinator_response_apphost_type"
                ]
            }
        },
        "nodes": {
            "MY_COMBINATOR": {
                "alias_config": {
                    "addr_alias": [
                        "HOLLYWOOD_ALL",
                        "MY_COMBINATOR"
                    ]
                },
                "backend_name": "HOLLYWOOD_COMMON",
                "node_type": "DEFAULT",
                "params": {
                    "handler": "/my_combinator/run",
                    "soft_timeout": "20ms",
                    "timeout": "50ms"
                }
            }
        }
    }
}
```

3. Собираем и запускаем [graph_generator](https://a.yandex-team.ru/arcadia/alice/apphost/graph_generator).
4. Пришем реализацию комбинатора в Голливуде в директории [combinators](https://a.yandex-team.ru/arcadia/alice/hollywood/library/combinators/combinators) по [примеру](https://a.yandex-team.ru/arcadia/alice/hollywood/library/combinators/combinators/centaur/centaur_main_screen.h). Нужно реализовать `Do` методы и в ответ положить айтем `combinator_response_apphost_type` с типом `TCombinatorRequest`. Хендрелы нужно зарегистрировать в голливуде вот [так](https://a.yandex-team.ru/arcadia/alice/hollywood/library/combinators/combinators/centaur/register.cpp). Также прописываем путь к своему комбинатору в [ya.inc](https://a.yandex-team.ru/arcadia/alice/hollywood/library/combinators/ya.inc).

{% note info "" %}

Используйте обертки и билдеры для [Request](https://a.yandex-team.ru/arcadia/alice/hollywood/library/combinators/request), [Response](https://a.yandex-team.ru/arcadia/alice/hollywood/library/combinators/response) и [AnalyticsInfo](https://a.yandex-team.ru/arcadia/alice/hollywood/library/combinators/analytics_info). Смело добавляйте нужные методы.

{% endnote %}

5. Запускаем локально apphost-mm-hw по [инструкции](https://docs.yandex-team.ru/alice-scenarios/quickstart#pervyj-shag-podnyat-servisy) или любым удобным вам способом.
6. Делаем запрос через [Аманду](https://docs.yandex-team.ru/alice-scenarios/testing/amanda). Megamind url укажите следующий: 
   ```
    http://vins.hamster.alice.yandex.net/speechkit/app/pa/?srcrwr=APP_HOST:nikitakodosov.sas.yp-c.yandex.net:40000&srcrwr=MY_COMBINATOR:nikitakodosov.sas.yp-c.yandex.net:12346&srcrwr=MEGAMIND_ALIAS:nikitakodosov.sas.yp-c.yandex.net:23341
   ```

На любой запрос в такой конфигурации Мегамайнд будет выбирать ответ комбинатора, который сформирован в п.4.

## Тесты

Для тестирования комбинаторов можно написать evo-тест ([пример](https://a.yandex-team.ru/arcadia/alice/tests/integration_tests/combinators?rev=r9551140)) или юнит-тест ([пример](https://a.yandex-team.ru/arcadia/alice/hollywood/library/combinators/combinators/centaur/centaur_main_screen_ut.cpp?rev=r9551140)).
It2 тесты пока не реализованы.
