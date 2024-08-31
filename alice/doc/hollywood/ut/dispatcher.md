# Тестирование функций диспетчера

Тестирование диспетчера позволяет убедиться, выбирает ли диспетчер корректную сцену и аргументы или иррелевантный рендер в зависимости от:
* параметров поверхности;
* флагов экспериментов; 
* пришедших семантических фреймов.

Для хорошего покрытия юнит-тестами диспетчера надо перебрать все варианты входных данных для выбора всех имеющихся сцен и иррелевантных ответов.

```c++
UNIT_ASSERT(testData >> TTestDispatch(&MyScenario::Dispatch) >> testResult);
```

Перед вызовом функции `TTestDispatch` в структуре `testData` может потребоваться заполнить протобафы `testData.RequestMeta` и/или `testData.RunRequest`.

Поля могут быть заполнены напрямую при помощи соответствующий функций протобафов или при помощи функций-хелперов.

{% cut "Примеры заполнения полей" %}

```c++
TTestEnvironment testData("my_scenario", "ru-ru");
// Для request meta - прямой доступ к протобафу
testData.RequestMeta.SetRequestId("12345);
// Для run request - прямой доступ к протобафу
testData.RunRequest.MutableBaseRequest()->MutableInterfaces()->SetHasScreen(true);
// С использованием дополнительных функций
testData.SetDeviceId("12345");
testData.AddExp("my_exp_flag=my_exp_value, "1");
...
UNIT_ASSERT(testData >> TTestDispatch(&TMyScenario::Dispatch) >> testResult);
```

Полный список см. в существующих юнит-тестах или в файле [test_environment.h](https://a.yandex-team.ru/arc_vcs/alice/hollywood/library/framework/unittest/test_environment.h).

{% endcut %}

Чтобы после прогона функции в `testResult` проверить, что диспетчер выбрал корректную сцену для обработки или вернул запрос на иррелевантный рендер, протестируйте поля `testResult` напрямую или при помощи функций-хелперов:

* `testResult.sceneName` — название выбранной сцены;
* `testResult.SceneArguments` — аргументы для сцены;
* `testResult.RenderArguments` — аргументы для функции рендера (если в ответ на сформированный запрос предполагается, что диспетчер вернул `irrelevant`).

```c++
...
UNIT_ASSERT(testData >> TTestDispatch(&TMyScenario::Dispatch) >> testResult);
//
// Примеры тестов, если предполагается, что диспетчер выбрал сцену
//
UNIT_ASSERT_STRINGS_EQUAL(testResult.sceneName, "my_scene_name");
TMySceneArgs sceneArgs;
UNIT_ASSERT(testData.SceneArguments.GetArguments().UnpackTo(&sceneArgs));
UNIT_ASSERT_STRINGS_EQUAL(sceneArgs.GetMyValue(), "my_value");
//
// Примеры тестов, если предполагается, что диспетчер выбрал иррелевантный рендер
//
TMySceneRender renderArgs;
UNIT_ASSERT(testResult.RenderArguments.GetArguments().UnpackTo(&renderArgs));
UNIT_ASSERT_STRINGS_EQUAL(renderArgs.GetMyValue(), "my_value");
```
