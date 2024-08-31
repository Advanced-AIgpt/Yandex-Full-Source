# lua-before-cache-save

Содержит Lua-код, который выполняется в случае отсутствия результатов работы блока в кэше, вслед за выполнением блока.

В теге обязательно должно быть указано пространство имен xscript:

```
<x:lua-before-cache-save>
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
* [lua-after-cache-load](../reference/lua-after-cache-load.md)