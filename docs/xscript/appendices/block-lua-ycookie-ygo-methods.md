# Методы класса xscript.ycookie.ygo

Класс `xscript.cookie.ygo` предназначен для работы с [(cуб)кукой ygo](http://wiki.yandex-team.ru/Cookies/Y#ygo), включаемой в [Y-куки](http://wiki.yandex-team.ru/Cookies/Y) ys и yp.

#### Список методов: 

- [devicetype](block-lua-ycookie-ygo-methods.md#devicetype);
- [new](block-lua-ycookie-ygo-methods.md#new);
- [parse](block-lua-ycookie-ygo-methods.md#parse);
- [serialize](block-lua-ycookie-ygo-methods.md#serialize);
- [precision](block-lua-ycookie-ygo-methods.md#precision);

#### `from([number])` {#devicetype}

Возвращает (если входной параметр опущен) или устанавливает числовой идентификатор предыдущего региона пользователя. 

#### `new()` {#new}

Конструктор. Создает объект, который соответствует куке ygo, не содержащей данных. 

#### `parse(cookie)` {#parse}

Производит разбор содержимого куки ygo и сохраняет полученные данные во внутренней структуре. Кука подается на вход в виде строки.

#### `serialize()` {#serialize}

Преобразует куку ygo из внутреннего представления в строковое и возвращает полученное значение.

#### `to([number])` {#precision}

Возвращает (если входной параметр опущен) или устанавливает числовой идентификатор текущего региона пользователя.

### Узнайте больше {#learn-more}
* [Lua-блок](../concepts/block-lua-ov.md)
* [lua](../reference/lua.md)