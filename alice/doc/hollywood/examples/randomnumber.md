# Генерация случайного числа

## Описание
* Сценарий обрабатывает запросы вида «загадай число (от X до Y)»:
    * Если дипазон обозначен, то отвечает случайным от X до Y.
    * Если диапазон не обозначен, то использует значения от 1 до 100.
* Сценарий обрабатывает запросы вида «а еще число»:
    * Отвечает новым значением в ранее заданном диапазоне.
* Сценарий обрабатывает запросы вида «брось кубик».

Ознакомьтесь с [полной версией сценария](https://a.yandex-team.ru/arc_vcs/alice/hollywood/library/scenarios/random_number) с комментариями.

## Состав сценария

* Диспетчер.
* 2 сцены.
* 2 функции рендера для ответов:
    * на запрос «загадай случайное число»;
    * на запрос «брось кубик».

## Сцены

* `Random` — загадай случайное число.
* `Dice` — брось кубик. 

Сцены опираются на структуры данных:
* `TRandomNumberSceneArgsRandom`
* `TRandomNumberSceneArgsDice`

```proto
// Scene arguments for default scene (from...to)
message TRandomNumberSceneArgsRandom {
    optional int64 LowerBound = 1;
    optional int64 UpperBound = 2;
}

// Scene arguments for dice scene (dice_count, edge_count)
message TRandomNumberSceneArgsDice {
    int64 DiceCount = 1;
    int64 EdgeCount = 2;
}
```
## Рендеры

Рендеры опираются на структуры данных:
* `TRandomNumberRenderStateRandom`
* `TRandomNumberRenderStateDice`

Структуры заполняются в процессе выполнения бизнес-логики внутри соответствующих сцен.

Поля `LowerBound` и `UpperBound` передаются в структуру для формирования специальных NLG-ответов. Например, если попросить загадать число от 10 до 10, Алиса ответит, что ей не оставили никакого выбора.

```proto
message TRandomNumberRenderState {
    int64 LowerBound = 1;
    int64 UpperBound = 2;
    int64 Value = 3;
}

message TRandomNumberRenderStateDice {
    int64 DiceCount = 1;
    repeated int64 Values = 2;
    int64 Sum = 3;
}
```
