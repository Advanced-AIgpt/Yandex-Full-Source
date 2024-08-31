# Методы объекта xscript.localargs

Методы объекта xscript.localargs позволяют получить доступ к содержимому контейнера [LocalArgs](../concepts/block-local-ov.md).

#### Список методов: 

- [get](block-lua-localargs-methods.md#get);
- [getAll](block-lua-localargs-methods.md#getall);
- [getTypedValue](block-lua-localargs-methods.md#get-typed-value);
- [has](block-lua-localargs-methods.md#has);
- [is](block-lua-localargs-methods.md#is).

#### `get(name, def_value)` {#get}

Возвращает строковое представление значения переменной с именем `name` из LocalArgs. При отсутствии такой переменной в контейнере возвращает значение `def_value`. В качестве значения _def_value_ может использоваться nil.

Параметр `def_value` опциональный. По умолчанию - пустая строка.

#### `getAll()` {#getall}

Возвращает содержимое контейнера [LocalArgs](../concepts/block-local-ov.md#localargs) в виде таблицы.

#### `getTypedValue(name)` {#get-typed-value}

Получает типизированное значение переменной с именем _name_ из контейнера LocalArgs. При ее отсутствии возвращает nil.

#### `has(name)` {#has}
Проверяет, есть ли в LocalArgs переменная с именем `name`. 

#### `is(name)` {#is}

Проверяет, что в LocalArgs есть переменная с именем `name` и значением "true" или переменной нет. Для строковой переменной значение "false" – это пустая строка. Для Double – это число в диапазоне от -2e-16 до +2e-16. Для всех остальных целочисленных типов и Boolean – это 0.


### Узнайте больше {#learn-more}
* [Lua-блок](../concepts/block-lua-ov.md)
* [lua](../reference/lua.md)
