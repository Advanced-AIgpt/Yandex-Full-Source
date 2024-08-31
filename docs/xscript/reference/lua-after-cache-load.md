# lua-after-cache-load

Содержит Lua-код, который выполняется после извлечения из кэша контейнера с метаданными и результата работы блока.

В теге обязательно должно быть указано пространство имен xscript:

```
<x:lua-after-cache-load>
```

## Содержит {#contains}

Не содержит других тегов.

## Содержится в {#contained-in}

[meta](meta-tag.md).

## Атрибуты {#attrs}

отсутствуют.

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
* [meta](../reference/meta-tag.md)
* [lua-before-cache-save](../reference/lua-before-cache-save.md)