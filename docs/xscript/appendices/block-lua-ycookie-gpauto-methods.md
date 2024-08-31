# Методы класса xscript.ycookie.gpauto

Класс `xscript.cookie.gpauto` предназначен для работы с [(cуб)кукой gpauto](http://wiki.yandex-team.ru/Geolocation/Cookies#kukagpautovkontejjnereys/cookies/y), включаемой в [Y-куки](http://wiki.yandex-team.ru/Cookies/Y) ys и yp.

#### Список методов: 

- [devicetype](block-lua-ycookie-gpauto-methods.md#devicetype);
- [latitude](block-lua-ycookie-gpauto-methods.md#latitude);
- [longitude](block-lua-ycookie-gpauto-methods.md#longitude);
- [new](block-lua-ycookie-gpauto-methods.md#new);
- [parse](block-lua-ycookie-gpauto-methods.md#parse);
- [serialize](block-lua-ycookie-gpauto-methods.md#serialize);
- [precision](block-lua-ycookie-gpauto-methods.md#precision);
- [timestamp](block-lua-ycookie-gpauto-methods.md#timestamp);

#### `devicetype([number])` {#devicetype}

Возвращает (если входной параметр опущен) или устанавливает числовой идентификатор типа устройства. 

#### `latitude([number])` {#latitude}

Возвращает (если входной параметр опущен) или устанавливает широту географической точки.

#### `longitude([number])` {#longitude}

Возвращает (если входной параметр опущен) или устанавливает долготу географической точки.

#### `new()` {#new}

Конструктор. Создает объект, который соответствует куке gpauto, не содержащей данных. 

#### `parse(cookie)` {#parse}

Производит разбор содержимого куки gpauto и сохраняет полученные данные во внутренней структуре. Кука подается на вход в виде строки.

#### `serialize()` {#serialize}

Преобразует куку gpauto из внутреннего представления в строковое и возвращает полученное значение.

#### `precision([number])` {#precision}

Возвращает (если входной параметр опущен) или устанавливает точность определения географических координат (в метрах).

#### `timestamp` {#timestamp}

Возвращает (если входной параметр опущен) или устанавливает время определения координат (UTC, UNIX time stamp).


### Узнайте больше {#learn-more}
* [Lua-блок](../concepts/block-lua-ov.md)
* [lua](../reference/lua.md)