# name

В [JavaScript-блоке](../concepts/block-js-ov.md) и [Lua-блоке](../concepts/block-lua-ov.md) содержит имя XML-элемента, в котором размещается результат работы блока.

В [CORBA-блоке](../concepts/block-corba-ov.md) содержит имя серванта, к которому выполняется запрос.

## Содержит {#contains}

Данный тег не может содержать других тегов.

## Содержится в {#contained-in}

[block](block.md), [js](js.md), [lua](lua.md).

## Атрибуты {#attrs}

Нет.

## Пример {#example}

```xml
<block threaded="no">
    <name>Yandex/Finder.id</name>
    <method>findObject</method>
    <param type="State"/>
    <param type="String">Yandex/Mist.id</param>
    <param type="String">objname</param>
</block>
```

