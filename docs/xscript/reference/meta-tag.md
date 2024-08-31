# meta

Содержит Lua-код для обработки [метаинформации](../concepts/meta.md) о вызове блока.

В теге _\<meta\>_ обязательно должно быть указано пространство имен xscript:

```
<x:meta>
```

В теге _\<meta\>_ можно указать XPath- и XPointer-выражения, которые применяются к результату обработки секции. По умолчанию применяется XPointer-выражение «/..», то есть результат обработки секции - пустой.

## Содержит {#contains}

[lua](lua.md), [lua-after-cache-load](lua-after-cache-load.md), [lua-before-cache-save](lua-before-cache-save.md), [xpath](xpath.md), [xpointer](xpointer-tag.md).

## Содержится в {#contained-in}

тегах любых [блоков](../concepts/block-ov.md).

## Атрибуты {#attrs}

**name** - имя корневого элемента результата обработки секции _\<meta\>_.

## Пример {#example}

```
<x:local proxy="yes" tag="yes">
    <root>
        <mist method="dumpState"/>
        <lua>xscript.meta:setString("key0", "value0")</lua>
    </root>
    <x:meta xpointer="/*">
        <x:lua-before-cache-save>
            xscript.meta:setExpireTime(1675389934);
        </x:lua-before-cache-save>
         <x:lua-after-cache-load>
            if xscript.state:has("expired")
                xscript.meta:setExpireTime(1);
            end
        </x:lua-after-cache-load>
        <x:lua>
            xscript.meta:setString("key1", "value1");
            xscript.meta:setString("key2", "value2");
        </x:lua>
    </x:meta>
</x:local>
```

### Узнайте больше {#learn-more}
* [Метаинформация о вызове блока](../concepts/meta.md)
* [lua-after-cache-load](../reference/lua-after-cache-load.md)
* [lua-before-cache-save](../reference/lua-before-cache-save.md)