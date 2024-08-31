# Тестирование функций полного графа

Тестирование полного графа позволяет проверить сразу несколько функций фреймворка за один вызов. 

## Тестирование однонодового графа

```c++
UNIT_ASSERT(testData >> TTestApphost("main") >> testResult);
```

В ноде `main` однонодового графа автоматически выполняются функции `Dispatch` + `Main` + `Render`, поэтому можно:

* Задать в `testData` параметры для диспетчера. Подробнее в разделе [Описание тестов для диспетчера](./dispatcher.md).
* Проверить ответ рендера в `testResult`. Подробнее в разделе [Описание тестов для рендера](./render.md).

```c++
TTestEnvironment testData("my_scenario", "ru-ru");
testData.RunRequest.MutableBaseRequest()->MutableInterfaces()->SetHasBluttooth(false);
testData.AddSemanticFrame("bluetooth_on", "[{\"name\":\"mode\",\"type\":\"int\",\"value\":\"1\"}]";

UNIT_ASSERT(testData >> TTestApphost("main") >> testResult);

UNIT_ASSERT(!testResult.IsIrrelevant());
UNIT_ASSERT(testResult.ContainsText("нет Bluetooth"));
```

## Тестирование двухнодового графа

В двухнодовом графе для полной проверки сценария последовательно вызовите две функции `TTestApphost()`. Названия нод должны соответствовать конфигурации аппхоста и декларации функции `SetApphostGraph()`. Подробнее в разделе [Актуализация графа аппхоста](../compatibility/migration.md#apphost-finalize).

```c++
UNIT_ASSERT(testData >> TTestApphost("run") >> testResult);
UNIT_ASSERT(testResult >> TTestApphost("main") >> testResult2);
```

1. Перед **первым вызовом** заполните `testData` как указано в примере выше.
2. После **второго вызова** проверьте результаты релевантного или иррелевантного рендеринга.

### Анализ и заполнение результатов сетевых походов

В примере выше из ноды `run` получили тестовое окружение `testResult`. Далее `testResult` передается во вторую ноду `main`.

Если сценарий использует сетевые походы, то между этими двумя вызовами можно добавить:
* проверку корректности заполнения параметров для сетевого похода;
* имитацию ответа источника, который будет использован в бизнес-логике сценария в ноде `main`.

```c++
UNIT_ASSERT(testData >> TTestApphost("run") >> testResult);

//
// Проверяем, что в ноде "run" функция Scene::SetupMain заполнила запрос в источник
// 
const google::protobuf::Message* msg = `testResult.FindSetupRequest("my_http_request")`
... // Распаковываем msg и проверяем, что аргументы для сетевого похода заданы корректность

//
// Готовим "ответы" источника, записываем их в тот же testResult
//
testResult.AddAnswer("my_http_response1", "\"status\":500");
NAppHostHttp::THttpResponse httpResponse; // создаем и заполняем структуру THttpResponse нужными данными
AddHttpAnswer("my_http_response2", httpResponse);

// Теперь функция Scene::Main() получит в TSource 2 ответа с ключами "my_http_response1" и "my_http_response2"
UNIT_ASSERT(testResult >> TTestApphost("main") >> testResult2);
```

После окончания работы ноды `main` выполнится рендер, и тест проверит итоговый ответ сценария. См. пример проверки в разделе [Тестирование однонодового графа](#testirovanie-odnonodovogo-grafa).
