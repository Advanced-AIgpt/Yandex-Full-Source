# Управление кэшированием с помощью \<meta\>

Управлять кэшированием можно, только если установлен атрибут блока `tag="yes"`. В противном случае значения _expires_ и _last modified_ не будут изменены.

Для управления кэшированием результатов работы блока можно использовать теги [\<lua-after-cache-load\>](../reference/lua-after-cache-load.md) и [\<lua-before-cache-save\>](../reference/lua-before-cache-save.md) в секции [\<meta\>](meta.md).

В `<lua-after-cache-load>` описывается Lua-код, который выполняется после извлечения из кэша контейнера с метаданными и результата работы блока. После окончания обработки Lua-кода XScript проверяет значение _expire time_ (время в unixtime, до которого валиден кэш), так как оно могло измениться во время выполнения кода в `<lua-after-cache-load>`. Если _expire time_ больше настоящего момента, данные в кэше признаются валидными. Далее выполняется Lua-код в [\<x:lua\>](meta.md#lua), производится XML-сериализация контейнера с метаинформацией и выполняются наложения XPath и XPointer. Если кэш невалиден, выполняется код блока, как при отсутствии данных в кэше.

В `<lua-before-cache-save>` описывается Lua-код, который выполняется перед сохранением результатов работы блока в кэше. После окончания обработки Lua-кода XScript проверяет значение _expire time_ (см. выше) и, если оно больше настоящего момента, кэширует ответ блока и контейнер с метаинформацией. Далее выполняется Lua-код в \<x:lua\>, производится XML-сериализация контейнера с метаинформацией и выполняются наложения XPath и XPointer.

Таким образом, при изменении _expire time_ во время обработки секции [\<meta\>](meta.md) изменяется время кэширования.

**Пример**:

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
