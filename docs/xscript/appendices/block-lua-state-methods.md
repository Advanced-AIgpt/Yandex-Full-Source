# Методы объекта xscript.state

#### Список методов:

- [drop](block-lua-state-methods.md#drop);
- [erase](block-lua-state-methods.md#erase);
- [get](block-lua-state-methods.md#get);
- [getAll](block-lua-state-methods.md#getall);
- [getTypedValue](block-lua-state-methods.md#get-typed-value);
- [has](block-lua-state-methods.md#has);
- [is](block-lua-state-methods.md#is);
- [setBool](block-lua-state-methods.md#set_bool);
- [setDouble](block-lua-state-methods.md#set_double);
- [setLong](block-lua-state-methods.md#set_long);
- [setLongLong](block-lua-state-methods.md#set_long_long);
- [setTable](block-lua-state-methods.md#set-table);
- [setString](block-lua-state-methods.md#set_string);
- [setULong](block-lua-state-methods.md#set_ulong);
- [setULongLong](block-lua-state-methods.md#set_ulonglong).

#### `drop(prefix)` {#drop}

При вызове без параметров полностью очищает контейнер State. При вызове с опциональным параметром `prefix` удаляет из State все переменные, имена которых начинаются с `prefix`.

**Пример использования**:

Удаление всех переменных из State:

```
<x:lua>
     <![CDATA[
         xscript.state:drop();
     ]]>
</x:lua>
```

Удаление переменных с префиксом "var". Например, var1, var2, и т.д.:

```
<x:lua>
     <![CDATA[
         xscript.state:drop('var');
     ]]>
</x:lua>
```

#### `erase(varname)` {#erase}

Удаляет переменную с именем `varname` из контейнера [State](../concepts/state-ov.md).

**Пример использования**:

```
<lua>
     xscript.state:erase('myvariable');
</lua> 
```

#### `get(name,default)` {#get}

Возвращает стоковое представление значения переменной с именем `name` из контейнера [State](../concepts/state-ov.md). При попытке получить переменную типа Array возвращает значение первого элемента массива, а для типа Map - значение первого элемента словаря.

В случае отсутствия запрошенной переменной в State, возвращается значение, переданное в параметре `default`. В качестве такого значения может использоваться nil.

#### `getAll()` {#getall}

Возвращает содержимое контейнера [State](../concepts/state-ov.md) в виде таблицы.

#### `getTypedValue(name)` {#get-typed-value}

Получает типизированное значение переменной с именем _name_ из контейнера [State](../concepts/state-ov.md). При ее отсутствии возвращает nil.

**Пример использования**:

```
<lua>
     local p = {a="s1", b="a2"}
     xscript.state:setTable("map_var", p)
     <!-- map_var - таблица -->
     local map_var = xscript.state:getTypedValue("map_var") 

     xscript.state:setLong("long_var", 1)
     <!-- long_var - целое число -->
     local long_var = xscript.state:getTypedValue("long_var") 
</lua>
```

#### `has(name)` {#has}

Проверяет, есть ли в State переменная с именем `name`.

#### `is(name)` {#is}

Проверяет, что в State есть переменная с именем `name` и значением "true" или переменной нет (по аналогии со свойством [guard](../reference/guard.md)). Для строковой переменной значение "false" – это пустая строка. Для Double – это число в диапазоне от -2e-16 до +2e-16. Для всех остальных целочисленных типов и Boolean – это 0.

#### `setBool(name,value)` {#set_bool}

Присваивает переменной с именем `name` значение `value` типа Boolean.

#### `setDouble(name,value)` {#set_double}

Присваивает переменной с именем `name` значение `value` типа Double.

#### `setLong(name,value)` {#set_long}

Присваивает переменной с именем `name` значение `value` типа Long.

#### `setLongLong(name,value)` {#set_long_long}

Присваивает переменной с именем `name` значение `value` типа LongLong.

#### `setString(name, value)` {#set_string}

Присваивает переменной с именем `name` строковое значение `value`.

#### `setTable(name, value)` {#set-table}

Присваивает переменной с именем `name` значение `value` типа Array или Map в зависимости от вида таблицы Lua.

**Пример**:

```
<lua>
     <!-- Переменной присваивается значение типа Array -->
     local f = {"s1", "s2"}
     xscript.state:setTable("array_var", f)

     <!-- Переменной присваивается значение типа Map -->
     local p = {a="s1", b="a2"}
     xscript.state:setTable("map_var", p) 
</lua>
```

#### `setULong(name,value)` {#set_ulong}

Присваивает переменной с именем `name` значение `value` типа ULong.

#### `setULongLong(name,value)` {#set_ulonglong}

Присваивает переменной с именем `name` значение `value` типа ULongLong.

**Пример использования**:

```
<x:lua>
      <![CDATA[
          for i = 1, 10 do
          local key = string.format("long %d", i)
          xscript.state:setLong(key, i)
          xscript.state:setString(string.format("string %d", i), string.format("%d", i * 2));
          xscript.state:setLongLong(string.format("long long %d", i), xscript.state:get(key) * 3)
          end
      ]]>
</x:lua>
```

### Узнайте больше {#learn-more}
* [Lua-блок](../concepts/block-lua-ov.md)
* [lua](../reference/lua.md)
