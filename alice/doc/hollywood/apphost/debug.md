# Отладка графа

Для отладки проблем при сбоях в работе графа используйте режим отладочной трассировки фреймворка. Чтобы его включить, поставьте второй опциональный параметр конструктора `TScenario` в `true`:

```cpp
TScenario::TScenario(const TStringBuf name, bool enableDebugGraph = false)
```

Режим отладочной трассировки включает следующие функции:
* предварительный дамп построенного аппхостового и локального графа сценария;
* трассировка вызовов локального графа сценария при приходе событий от аппхоста.


{% note warning %}

Не комиттьте режим отладочной трассировки в `trunk`. Используйте его только для отладки графов при локальном запуске сервера Голливуда.

{% endnote %}

## Предварительный дамп

Выводится в `stdout` при старте сервера Голливуда, содержит подробную информацию о сконструированных графах. 

```
Scenario 'random_number' graph information:
  Apphost node: '/run'
    Local graph node: Dispatch
    Local graph node: SceneMainRun
    Local graph node: Render
    Local graph node: Finalize
  Apphost node: '/run'; Experiment: random_number_2node
    Local graph node: Dispatch
    Local graph node: SceneSetupRun
    Local graph node: Bypass
  Apphost node: '/main'
    Local graph node: SceneMainRun
    Local graph node: Render
    Local graph node: Finalize
```

В дампе видно, что сценарий зарегистрировал 3 аппхостовые ноды. Нода `/run` по умолчанию содержит все локальные ноды от диспатча до рендера и последующей финализации (нода `Finalize` готовит ответ для [Megamind](../../megamind/index.md)).

Кроме этого под флагом `random_number_2node` работает альтернативный аппхостовый граф на 2 ноды: `/run` и `/main`.

Нода `/run` содержит узлы диспетчера и сетапа сцены (при его наличии).
Нода `/main` содержит бизнес логику выбранной сцены и рендер.

Обратите внимание, что первая нода заканчивается `Bypass` (актуального ответа в Megamind еще нет, готовятся данные для сцен и опционального сетевого похода).

## Трассировка вызовов

Трассировка вызовов производится на уровне протоколирования `LOG_DEBUG` из файла `scenario_factory.cpp`. Пример трассировки для двухнодового графа:

Лог вызова первой ноды `/run` под экспериментом. Так как сценарий не содержит сетевых походов и функция `SceneSetupRun` отсутствует, эта фаза будет пропущена.
После работы диспетчера в логе также появляется информация о выбранной сцене.

```
scenario_factory.cpp:247 Scenario 'random_number' activated from apphost node '/run'
scenario_factory.cpp:252 Selected apphost graph: 'run', exp: random_number_2node
scenario_factory.cpp:262 Preparing to call a node 'Dispatch'; Scene: N/A
scenario_factory.cpp:262 Preparing to call a node 'SceneSetupRun'; Scene: default
scenario_factory.cpp:268 Node 'SceneSetupRun' skipped because it's absent in local graph declaration
scenario_factory.cpp:262 Preparing to call a node 'Bypass'; Scene: default
scenario_factory.cpp:272 Exiting from local graph with result: Success
```

Для вызова второй ноды `/main` эксперимент можно уже не указывать, так как кроме как из экспериментального `run` в нее не попасть:

```
scenario_factory.cpp:247 Scenario 'get_date' activated from apphost node '/main'
scenario_factory.cpp:252 Selected apphost graph: 'main', exp:
scenario_factory.cpp:262 Preparing to call a node 'SceneMainRun'; Scene: default
scenario_factory.cpp:262 Preparing to call a node 'Render'; Scene: default
scenario_factory.cpp:262 Preparing to call a node 'Finalize'; Scene: default
scenario_factory.cpp:272 Exiting from local graph with result: Success
```
