# Методы объекта xscript.meta

Методы объекта _xscript.meta_ позволяют получить доступ к содержимому [контейнера с метаинформацией](../concepts/meta.md) о вызове блока.

#### Список методов:

- [get](block-lua-meta.md#get);
- [getElapsedTime](block-lua-meta.md#get-elapsed-time);
- [getExpireTime](block-lua-meta.md#get-expire-time);
- [getLastModified](block-lua-meta.md#get-last-modified);
- [getTypedValue](block-lua-meta.md#get-typed-value);
- [setBool](block-lua-meta.md#set-bool);
- [setExpireTime](block-lua-meta.md#set-expire-time);
- [setLong](block-lua-meta.md#set-long);
- [setLongLong](block-lua-meta.md#set-longlong);
- [setString](block-lua-meta.md#set-string);
- [setTable](block-lua-meta.md#set-table);
- [setULong](block-lua-meta.md#set-ulong);
- [setULongLong](block-lua-meta.md#set-ulonglong).

#### `get(name, def_value)` {#get}

Возвращает строковое значение переменной с именем _name_ из [контейнера с метаинформацией](../concepts/meta.md) о работе блока, а в случае отсутствия такой переменной - значение _def_value_. В значения _def_value_ может использоваться nil.

При попытке получить переменную типа Array возвращает значение первого элемента массива, а для типа Map - значение первого элемента словаря.

Параметр _def_value_ опционален и по умолчанию равен пустой строке.

#### `getElapsedTime()` {#get-elapsed-time}

Возвращает время выполнения блока с перблочным преобразованием в миллисекундах.

Если на момент запроса выполнение блока еще не завершено, возвращает -1.

#### `getExpireTime()` {#get-expire-time}

Возвращает время, до которого валиден закэшированный ответ блока, в формате unixtime.

Если копии в кэше нет, возвращает -1. Если время не определено, возвращает 0.

#### `getLastModified()` {#get-last-modified}

Возвращает время последнего изменения закешированной копии ответа блока в формате unixtime.

Если копии в кэше нет, возвращает -1. Если время не определено, возвращает 0.

#### `getTypedValue(name)` {#get-typed-value}

Получает типизированное значение переменной с именем _name_ из [контейнера с метаинформацией](../concepts/meta.md) о вызове блока. При ее отсутствии возвращает nil.

#### `setBool(name, value)` {#set-bool}

Присваивает переменной с именем _name_ значение _value_ типа Boolean.

#### `setExpireTime(time)` {#set-expire-time}

Устанавливает время _time_ (в формате unixtime), до наступления которого будет валиден закэшированный результат работы блока.

Время будет установлено, только если блок может кэшироваться и для него установлен атрибут `tag="yes"`.

Возвращает время, до которого будет валиден кэш.

#### `setLong(name, value)` {#set-long}

Присваивает переменной с именем _name_ значение _value_ типа Long.

#### `setLongLong(name, value)` {#set-longlong}

Присваивает переменной с именем _name_ значение _value_ типа LongLong.

#### `setString(name, value)` {#set-string}

Присваивает переменной с именем _name_ значение _value_ типа String. 

#### `setTable(name, value)` {#set-table}

Присваивает переменной с именем _name_ значение _value_ типа Array или Map в зависимости от вида таблицы Lua.

#### `setULong(name, value)` {#set-ulong}

Присваивает переменной с именем _name_ значение _value_ типа ULong.

#### `setULongLong(name, value)` {#set-ulonglong}

Присваивает переменной с именем _name_ значение _value_ типа ULongLong.

**Пример использования**:

```
<x:local proxy="yes">
    <root>
        <mist method="dumpState"/>
        <lua>xscript.meta:setString("key0", "value0")</lua>
    </root>
    <x:meta xpointer="/*">
        <xpath expr="/meta/key0" result="key"/>
        <x:lua>
            xscript.meta:setString("key1", "value1")
            xscript.meta:setString("key2", "value2")
            local t = {a=1, b=2}
            xscript.meta:setTable("key3", t)
            local p = {3, 4}
            xscript.meta:setTable("key4", p)
        </x:lua>
    </x:meta>
</x:local>
```

### Узнайте больше {#learn-more}
* [Метаинформация о вызове блока](../concepts/meta.md)
* [meta](../reference/meta-tag.md)