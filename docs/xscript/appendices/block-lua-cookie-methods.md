# Методы класса xscript.cookie

#### Список методов: 

- [domain](block-lua-cookie-methods.md#domain);
- [expires](block-lua-cookie-methods.md#expires);
- [httpOnly](block-lua-cookie-methods.md#httponly);
- [name](block-lua-cookie-methods.md#name);
- [new](block-lua-cookie-methods.md#new);
- [path](block-lua-cookie-methods.md#path);
- [permanent](block-lua-cookie-methods.md#permanent);
- [secure](block-lua-cookie-methods.md#secure);
- [value](block-lua-cookie-methods.md#value).

Большинство методов класса  существуют в двух вариантах:
 
 
- `xscript.cookie.{field}()` (без параметров)  — возвращает текущее значение поля;
- `xscript.cookie.{field}(value)` (с параметром) — устанавливает значение поля.

Исключение составляют методы `name` и `value`, которые не могут изменять имя и значение куки соответственно.

#### `domain()` {#domain}
 
Возвращает или устанавливает значение атрибута куки `domain`.

#### `expires()` {#expires}

Возвращает или устанавливает время в Unix time, до которого будет валидна кука.

#### `httpOnly()` {#httponly}

Возвращает или устанавливает значение атрибута куки `httpOnly`.

#### `name()` {#name}

Возвращает имя куки.

#### `new(name, value)` {#new}

Создает новую куку с именем `name` и значением `value`. Кука по умолчанию является сессионной. Для установки времени жизни можно воспользоваться методами `expires` и `permanent`. 

#### `path()` {#path}

Возвращает или устанавливает значение атрибута куки path.

#### `permanent()` {#permanent}

При вызове `permanent(true)` устанавливает время жизни куки до 2038 года.

При вызове без параметров возвращает значение `true`, если установлено время жизни куки до 2038 года. В противном случае возвращает значение `false`.

#### `secure()` {#secure}

Возвращает или устанавливает для куки флаг `secure`.

#### `value()` {#value}

Возвращает значение куки.

**Пример использования**:

```javascript
c = xscript.cookie.new('foo', 'bar')
print(c:name())
c:path("/some/path")
c:domain(".example.com")
c:secure(true)
c:expires(123456789)
print("path:", c:path())
xscript.response:setCookie(c)
```

В данном примере создаётся кука foo со значением "bar", задаются её дополнительные параметры, после чего она отправляется пользователю.

### Узнайте больше {#learn-more}
* [Lua-блок](../concepts/block-lua-ov.md)
* [lua](../reference/lua.md)
