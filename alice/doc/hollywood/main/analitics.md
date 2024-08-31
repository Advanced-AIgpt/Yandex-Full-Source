# Аналитика

Каждый сценарий должен снабдить возвращаемый результат дополнительными данными для анализа структуры и качества ответа. Данные позволяют анализировать процессы общения Алисы с пользователями, выяснять полноту ответов и необходимости дозапросов пользователей.

Добавьте в свой сценарий специальную аналитику, в дополнение к стандартной — ее Hollywood Framework добавляет автоматически.

## Стандартная аналитика

Стандартные поля заполняются автоматически:
* `ProductScenarioName` (PSN)
* `IntentName`

### PSN

По умолчанию PSN совпадает с именем сценария (первый аргумент конструктора `TScenario`). Это имя можно переопределить двумя способами:
* **Чтобы предопределить PSN для всех запросов.** Вызвать в конструкторе сценария метод `void TScenario::SetProductScenarioName(TStringBuf productScenarioName)`;
* **Чтобы предопределить PSN для отдельных сцен.** вызвать в конструкторе отдельной сцены метод `void TScene::SetProductScenarioName(TStringBuf productScenarioName)`.

Если сцена не переопределила PSN, то будет использоваться PSN сценария.

PSN заполняется в [TAnalyticsInfo::ProductScenarioName](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/scenarios/analytics_info.proto?rev=r9094029#L244).

### IntentName

`IntentName` — название семантического фрейма, который сматчился при анализе запроса пользователя. Название сматчившегося интента Hollywood Framework добавляет автоматически при выборе сцены из диспетчера (третий аргумент функции `TReturnValueScene()`).

```cpp
return TReturnValueScene(pointerToSceneFn, sceneFnPrototype, const TString& selectedSemanticFrame = "")
```

Если вы при выборе сцены указали название сматчившегося фрейма, то по окончании обработки запроса фреймворк:
1. Добавит `selectedSemanticFrame` как [TAnalyticsInfo::Intent](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/scenarios/analytics_info.proto?rev=r9094029#L235) к блоку данных аналитики.
2. Добавит содержимое сматчившегося фрейма в [TScenarioResponseBody::SemanticFrame](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/scenarios/response.proto?rev=r9094029#L276).

Переменная `selectedSemanticFrame` не обязательно должна содержать актуальное имя фрейма, это может быть произвольное название, согласованное с аналитиками. 
В этом случае поле `TAnalyticsInfo::Intent` будет заполнено, но в `TScenarioResponseBody::SemanticFrame` будет пусто.

Независимо от того, что будет возвращено в переменной `selectedSemanticFrame`, диспетчер и выбранная сцена могут использовать данные одновременно из нескольких семантических фреймов, если таковые пришли в сценарий.

## Обратная связь
* [Чат аналитиков](https://t.me/joinchat/BFRY1hM0SrVZ4BcKZUYIhA)
* [Очередь VA](https://st.yandex-team.ru/VA/)