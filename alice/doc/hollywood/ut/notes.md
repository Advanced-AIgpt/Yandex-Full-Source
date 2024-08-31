# Дополнительные замечания про юнит-тесты

## Переиспользование переменных `TTestEnvironment`

Во всех примерах выше использовались две переменные `testData` и `testResult`:

```c++
TTestEnvironment testData("my_scenario", "ru-ru");
TTestEnvironment testResult(&testData);
//... заполняем testData
UNIT_ASSERT(testData >> TTestDispatch(&MyScenario::Dispatch) >> testResult);
//.. проверяем testResult
```

Переменная `testResult` и ее тип `TTestEnvironment` могут быть явно использованы для передачи в следующую ноду. Это позволяет использовать конструкции следующего вида:

```c++
TTestEnvironment testData("my_scenario", "ru-ru");
//... заполняем testData
UNIT_ASSERT(testData >> TTestDispatch(&MyScenario::Dispatch) >> testData);
//.. проверяем testData
```

Такой способ перегрузки является безопасным и позволяет использовать только одну переменную без дополнительных деклараций `TTestEnvironment testResult(&testData);`.

Прием может быть использован и при тестировании нескольких функций подряд:

{% cut "Примеры" %}

```c++
TTestEnvironment testData("my_scenario", "ru-ru");
...
UNIT_ASSERT(testData >> TTestDispatch(&TMyScenario::Dispatch) >> testData);
UNIT_ASSERT(testData >> TTestScene(&TMyScene::Main, sceneArguments) >> testData);
```

```c++
TTestEnvironment testData("my_scenario", "ru-ru");
...
UNIT_ASSERT(testData >> TTestApphost("run") >> testData);
UNIT_ASSERT(testData >> TTestApphost("main") >> testData);
```
{% endcut %}

* Если подряд выполняется несколько тестов `test >> TTestXxx() >> test` и содержимое `testData` должно остаться неизменным, используйте `testData` + `testResult`.
* Если в юнит-тесте только один вызов `test >> TTestXxx() >> test`, то исходную переменную `testData` можно переиспользовать и для выхода.

## Блокировка неверных ответов в stderr

Если вы сознательно тестируете сценарий на ошибочные ответы, чтобы не засорять `stderr` сообщениями, используйте метод `TTestEnvironment::DisableErrorReporting()` — он заблокирует дамп ошибок в логи.

Чтобы проверять работу сценариев в экстремальных условиях для тестирования функций полного графа в методах `AddAnswer()`/`AddHttpAnswer()`, добавьте ответы с ошибками (404, 500 и т.п.).

## Примеры тестов

Посмотрите примеры тестирования для сценария [random_number](https://a.yandex-team.ru/arc/trunk/arcadia/alice/hollywood/library/scenarios/random_number).
