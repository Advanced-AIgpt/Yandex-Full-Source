# param

Содержит параметр метода, вызываемого в XScript-блоке любого типа.

В случае использования в [Local-блоке](../concepts/block-local-ov.md) содержит параметры, которые передаются на вход вызываемому скрипту.

Параметры [File-блока](../concepts/block-file-ov.md) и [HTTP-блока](../concepts/block-http-ov.md) могут конкатенироваться. Если метод принимает несколько параметров, в первую очередь из списка параметров для конкатенации отбрасываются параметры Tag. Среди оставшихся параметров отбрасываются последние n-1 параметров, где n - количество параметров, которые принимает метод. Остальные параметры конкатенируются.

## Содержит {#contains}

Данный тег не может содержать других тегов.

## Содержится в {#contained-in}

[auth-block](auth-block.md), [banner-block](banner-block.md), [block](block.md), [custom-morda](custom-morda.md), [file](file.md), [geo](geo.md), [http](http.md), [local](local.md), [lua](lua.md), [mist](mist.md).

## Атрибуты {#attrs}

#|
|| Наименование | Описание | Тип и варианты значения | Значение по умолчанию ||
|| type | Тип параметра вызываемого метода. | См. [Все типы параметров методов, вызываемых в XScript-блоках](../appendices/block-param-types.md) | - ||
|| as | Тип, к которому необходимо привести значение параметра. В атрибуте `type` указывается приводимый тип (адаптер), а в атрибуте `as` - простой тип, к которому XScript приведёт значение. | Название одного из простых типов параметров: Boolean, Double, Long, LongLong, String, ULong, ULongLong. | String ||
|| default | Значение параметра по умолчанию. Используется, если значение не задано явно или его невозможно извлечь из объекта или структуры. | Зависит от типа параметра. | - ||
|| id | Используется только в [Local-блоке](../concepts/block-local-ov.md), где является обязательным атрибутом.

Имя параметра, передаваемого на вход скрипту. | Строка. | - ||
|#

## Пример {#example}

```
<block>
    <name>Yandex/Hyper.id</name>
    <method>search_in_subtree</method>
    <param type="QueryArg" as="String">task-name</param>
    <param type="QueryArg" as="Long">vendor-id</param>
    <param type="QueryArg" as="Long" default="0">page</param>
    <param type="Long">20</param>
    <param type="Long">0</param>
    <param type="String"></param>
</block>
```

## Сокращенный синтаксис {#syntax}

Если параметр метода имеет контейнерный тип, ключ в контейнере можно не указывать. В этом случае будет использоваться ключ, совпадающий со значением атрибута `id`. Например, запись

`<param id="gid" type="QueryArg"/>`

будет аналогична записи

`<param id="gid" type="QueryArg">gid</param>`.


### Узнайте больше {#learn-more}
* [Понятие XScript-блока и его типы](../concepts/block-ov.md)
* [Типы параметров](../concepts/parameters-ov.md)
* [method](../reference/method.md)