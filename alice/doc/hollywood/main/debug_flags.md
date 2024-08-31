# Отладочные флаги

Hollywood framework имеет ряд внутренних экспериментов, которые можно использовать для дампа основной информации в лог.

Дамп ведется в формате JSON на уровне протоколирования `LOG_INFO`.

Все флаги экспериментов имеют единый формат записи:

```
hwf_debug_dump_xxx=scenario_name[,extra_arguments]
```

где
* hwf_debug_dump_xxx - флаг эксперимента (см ниже).
* scenario_name - название вашего сценария в формате `lower_case` (как в конструкторе `TScenario`).
* extra_arguments - дополнительные опциональные параметры (зависят от флага, см ниже).

По умолчанию указанные протобафы выводятся в формате Proto. Для вывода протобафов в формате JSON используйте флаг `hwf_debug_dump_format=json`

## hwf_debug_dump_run_request

Выводит в лог исходный протобаф TScenarioRunRequest

```
hwf_debug_dump_request=scenario_name
```

## hwf_debug_dump_run_response

Выводит в лог результирующий протобаф TScenarioRunResponse

```
hwf_debug_dump_response=scenario_name
```

## hwf_debug_dump_continue_request

Выводит в лог исходный протобаф TScenarioContinueRequest

```
hwf_debug_dump_continue_request=scenario_name
```


## hwf_debug_dump_continue_response

Выводит в лог результирующий протобаф TScenarioContinueResponse

```
hwf_debug_dump_continue_response=scenario_name
```


## hwf_debug_dump_apply_request

Выводит в лог исходный протобаф TScenarioApplyRequest.

```
hwf_debug_dump_apply_request=scenario_name
```

## hwf_debug_dump_apply_response

Выводит в лог результирующий протобаф TScenarioApplyResponse

```
hwf_debug_dump_apply_response=scenario_name
```

## hwf_debug_dump_commit_request

Выводит в лог исходный протобаф TScenarioCommitRequest. Этот протобаф используется для ручки `/commit`

```
hwf_debug_dump_commit_request=scenario_name
```

## hwf_debug_dump_commit_response

Выводит в лог результирующий протобаф TScenarioCommitResponse

```
hwf_debug_dump_commit_response=scenario_name
```

## hwf_debug_dump_render_data

Выводит в лог протобаф "render_data", который был включен в ответ сценария в фазе `Main()` или `Render()`

```
hwf_debug_dump_render_data=scenario_name
```

## hwf_debug_dump_scene_arguments

Выводит в лог протобаф с аргументами сцены.

```
hwf_debug_dump_scene_arguments=scenario_name
или (если надо выводить аргументы сцены только для конкретно выбранной сцены)
hwf_debug_dump_scene_arguments=scenario_name,scene_name
```

## hwf_debug_dump_render_arguments

Выводит в лог протобаф с аргументами рендера.

```
hwf_debug_dump_render_arguments=scenario_name
или (если надо выводить аргументы рендера только для конкретно выбранной сцены)
hwf_debug_dump_render_arguments=scenario_name,scene_name
```

## hwf_debug_dump_continue_arguments

Выводит в лог протобаф с аргументами, которые Main()-функция задала как аргументы для Continue.

```
hwf_debug_dump_continue_arguments=scenario_name
или (если надо выводить аргументы только для конкретно выбранной сцены)
hwf_debug_dump_continue_arguments=scenario_name,scene_name
```

## hwf_debug_dump_commit_arguments

Выводит в лог протобаф с аргументами, которые Main()-функция задала как аргументы для Commit.

```
hwf_debug_dump_commit_arguments=scenario_name
или (если надо выводить аргументы только для конкретно выбранной сцены)
hwf_debug_dump_commit_arguments=scenario_name,scene_name
```

## hwf_debug_dump_apply_arguments

Выводит в лог протобаф с аргументами, которые Main()-функция задала как аргументы для Apply.

```
hwf_debug_dump_apply_arguments=scenario_name
или (если надо выводить аргументы только для конкретно выбранной сцены)
hwf_debug_dump_apply_arguments=scenario_name,scene_name
```

## hwf_debug_dump_datasource

Выводит в лог содержимое datasource.

```
hwf_debug_dump_datasource=scenario_name,datasource_name
```

Параметр datasource_name должен быть записан в формате `BLACK_BOX`, `USER_LOCATION`, и т.п.

## hwf_debug_dump_local_graph

Включает режим внутренней трассировки локального графа HWF. 
Этот флаг эквивалентен вызову функции EnableDebugGraph() в конструкторе сценария.

```
hwf_debug_dump_local_graph=scenario_name
```
