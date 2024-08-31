# xpointer

Позволяет наложить `XPointer`-выражение на результат работы любого блока для выборки из XML части узлов.

Подробнее об `XPointer` см. [Атрибут xpointer](../appendices/xpointer.md).

Существует возможность использовать в качестве выражения `XPointer` значение переменных из контейнеров [State](../concepts/state-ov.md) и [LocalArgs](../concepts/block-local-ov.md). Для этого в качестве атрибута `type` тега `<xpointer>` требуется указать `StateArg` и `LocalArg` соответственно, а в качестве значения — имя переменной, как показано ниже в примерах:

```
<mist method="set_state_string">
   <param type="String">xp</param>
   <param type="String">/page/b</param>
</mist>
<file>
   <xpointer type="StateArg">xp</xpointer>
   <method>load</method>
   <param type="String">file.xml</param>
</file>
```

```
<page xmlns:x="http://www.yandex.ru/xscript">
<x:local>
    <param id="xslt-local" type="String">./simple-stub.xsl</param>
    <param id="xp-local" type="String">head/title</param>

    <root name="toor">
        <file method="load">
            <param type="String">page.xml</param>
            <xslt type="LocalArg">xslt-local</xslt>
            <xpointer type="LocalArg">xp-local</xpointer>
        </file>
    </root>
</x:local>
</page>
```

## Содержит {#contains}

Данный тег не может содержать других тегов.

## Содержится в {#contained-in}

[auth-block](auth-block.md), [banner-block](banner-block.md), [block](block.md), [custom-morda](custom-morda.md), [file](file.md), [geo](geo.md), [http](http.md), [local](local.md), [lua](lua.md), [mist](mist.md), [while](while.md).

## Атрибуты {#attrs}

#|
|| Наименование | Описание | Тип и варианты значения | Значение по умолчанию ||
|| type | Может либо отсутствовать, либо иметь значения `StateArg` или `LocalArg`, являющиеся индикатором того, что должно быть выполнено `XPointer`-выражение, содержащееся в переменной контейнера соответствующего типа, имя которой указано в теге `<xpointer>`. | `StateArg` или `LocalArg` | - ||
|#


### Узнайте больше {#learn-more}
* [Атрибут xpointer](../appendices/xpointer.md)