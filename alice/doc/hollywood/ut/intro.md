# Введение

> *Комментарии и замечания по документации присылайте [Дмитрию Долгову](https://staff.yandex-team.ru/d-dima) ([@dDIMA](https://telegram.me/dDIMA)).*

## Типы тестирования

Способы тестирования сценариев Hollywood Framework:

* Юнит-тесты (ut)
* [Интеграционные тесты](https://wiki.yandex-team.ru/alice/hollywood/integrationtests/) (it2)
* [EVO-тесты](https://wiki.yandex-team.ru/alice/infra/integrationtests/) (evo)

Эти тесты имеют разную структуру и предназначены для проверки работоспособности сценария в разных вариантах.

## Отличия способов тестирования

Юнит-тесты — это простой и быстрый способ проверить работу вашего сценария. Главное отличие юнит-тестов от интеграционных и EVO-тестов:

* не требуют развертывания полной инфраструктуры (полный hollywood, mm, apphost); 
* позволяют проверять работу отдельных функций и всего сценария;
* быстро собираются.

| Юнит-тесты           | Интеграционные тесты          | EVO-тесты         |
| ------------- | ------------- | ------------- |
| Быстрая проверка вашего сценария    | Комплексная проверка работы сценария с инфраструктурой и стабами | Комплексная проверка работы сценария с инфраструктурой     |
| Контроль за определенными значениями переменных | Сравнение ответов с предыдущими результатами | Комплексная проверка ответов сценариев     |
| Тестирование функций      | Тесты сценария в целом от запроса (request) до ответа (response)     | Тесты сценария в целом от запроса (request) до ответа (response)    |
| Возможность задания специальных условий на входе | Эмуляция только реальных запросов на реальных конфигурациях поверхностей     | Эмуляция только реальных запросов на реальных конфигурациях поверхностей  |
| Генерация произвольных ответов источников        | Стандартные стабы вместо честных сетевых походов | Полная честная работа с источниками |

Hollywood Framework предоставляет дополнительную инфраструктуру к классическим вариантам юнит-тестов.

Для тестирования сценариев в юнит-тестах используются файлы:
* `alice/hollywood/library/framework_ut.h`
* `PEERDIR(alice/hollywood/library/framework/unittest)`

## Юнит-тесты для Hollywood Framework

Hollywood Framework предоставляет возможность автономного тестирования:

* функций диспетчера, сцен или рендеров;
* полного локального графа фреймворка до первого сетевого похода или до окончания рендера;
* полного графа сценария от стартового запроса до финального ответа.

Порядок тестирования:

1. Создайте структуру `TTestEnvironment`.
2. Заполните `TTestEnvironment` данными, которые необходимы для вашего теста.
3. Вызовите нужный метод обработки сценария.
4. Проверьте результат работы в результирующей структуре.

```c++
TTestEnvironment testData("my_scenario", "ru-ru");
TTestEnvironment testResult(testData);

// заполнить при необходимости входные данные (testData)
// пример для Semantic Frames:
testData.AddSemanticFrame("alice.frame.my_frame", "[{\"name\":\"query\",\"type\":\"string\",\"value\":\"day\"}]";

// вызвать одну из функций тестирования в формате testData >> TTestXxx(function_name, arguments) >> testResult;
testData >> TTestDispatch(&TMyScenario::Dispatch) >> testResults;

// проверить результаты в testResult (например, что выбрана корректная сцена)
UNIT_ASSERT(testResult.SceneName == "my_scene");
```

`TTestXxx` — это синтаксическое обозначение следующих классов:

* `TTestDispatch(pointer to scenario dispatcher)` — тестирование функций диспетчера (использована в примере выше).
* `TTestScene(pointer to scene, scene args)` — тестирование функций сцен.
* `TTestRender(pointer to renderer, renderer args)` — тестирование функций рендера.
* `TTestApphost(name of apphost node)` — полное тестирование локального графа фреймворка.

Выражение `testData >> TTestXxx(function_name, arguments) >> testResult;` возвращает итоговый результат:

* `true` — ответ получен;
* `false` — сценарий или его часть не найден, или случился `HW_ERROR()`. Это выражение можно целиком обернуть в макрос `UNIT_ASSERT()`:

```c++
TTestEnvironment testData("my_scenario", "ru-ru");
TTestEnvironment testResult(testData);
testData.AddSemanticFrame("alice.frame.my_frame", "[{\"name\":\"query\",\"type\":\"string\",\"value\":\"day\"}]";
// Проверяем, что тестирование диспетчера прошло корректно
UNIT_ASSERT(testData >> TTestDispatch(&TMyScenario::Dispatch) >> testResults);
```

```c++
TTestEnvironment testData("my_scenario", "ru-ru");
TTestEnvironment testResult(testData);
// Эта строка нужна для того, чтобы предотвратить "мусорный" дамп в stderr в процессе работы сценария.
testData.DisableErrorReporting(); 
testData.AddSemanticFrame("alice.frame.my_frame", "[{\"name\":\"baddata\",\"type\":\"\",\"value\":\"baddata\"}]";
// Проверяем, что тестирование диспетчера закончилось выбросом исключения HW_ERROR()
UNIT_ASSERT(!(testData >> TTestDispatch(&TMyScenario::Dispatch) >> testResults));
```
