# Тестирование функций сцен

Тестирование функций сцены позволяет убедиться, что в зависимости от входных данных и аргументов бизнес-логика сцены формирует правильные данные для рендера.

Для хорошего покрытия юнит-тестами нужно сформировать различные входные аргументы сцен (протобафы) и просмотреть, какие протобафы были сгенерированы для рендера.

```c++
UNIT_ASSERT(testData >> TTestScene(&MyScene::Main, SceneArgsProto) >> testResult);
```

Для тестирования заполните:

* Протобаф с аргументами `SceneArgsProto`, как будто его заполняет диспетчер.
* Структуру `testData`. Если тесты на сцену используют не только аргументы, но и поля в `TRunRequest`, сцена сама проверит флаги экспериментов.

По окончании работы в переменной `testResult` проверяется поле `testResult.RenderArguments` с аргументами для рендера.

{% cut "Пример" %}

```c++
// Создаем и заполняем протобаф с аргументами сцены
SceneArgsProto sceneArgsProto;
sceneArgsProto.SetField1("value");
sceneArgsProto.SetField2(123);
...
// Создаем и заполняем TestEnvirinment
TTestEnvironment testData("my_scenario", "ru-ru");
TTestEnvironment testResult(testData);
testData.AddExp("my_experiment", "1"); // Если требуется для успешной работы функций сцены, хотя желательно
                                       // все такие проверки совершать на уровне диспетчера.
...

// Прогоняем тесты на функцию сцены
UNIT_ASSERT(testData >> TTestScene(&MyScene::Main, sceneArgsProto) >> testResult);

// Проверяем результат:
TMySceneRender renderArgs;
UNIT_ASSERT(testResult.RenderArguments.GetArguments().UnpackTo(&renderArgs));
UNIT_ASSERT_STRINGS_EQUAL(renderArgs.GetMyValue(), "my_value");
```
{% endcut %}