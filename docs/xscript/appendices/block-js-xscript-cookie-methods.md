# Класс xscript.cookie

Класс для работы с кукой.

#### Список методов: 

- [domain](block-js-xscript-cookie-methods.md#domain);
- [expires](block-js-xscript-cookie-methods.md#expires);
- [httpOnly](block-js-xscript-cookie-methods.md#httponly);
- [name](block-js-xscript-cookie-methods.md#name);
- [path](block-js-xscript-cookie-methods.md#path);
- [permanent](block-js-xscript-cookie-methods.md#permanent);
- [secure](block-js-xscript-cookie-methods.md#secure);
- [value](block-js-xscript-cookie-methods.md#value).

Конструктор класса имеет вид `xscript.cookie (name, value)`, где `name` — имя куки, `value` — ее значение. При работе с экземпляром класса значения этих параметров могут быть получены с помощью методов [`name`](block-js-xscript-cookie-methods.md#name) и [`value`](block-js-xscript-cookie-methods.md#value), но не могут быть изменены.

По умолчанию кука объявляется сессионной. Для установки времени жизни можно воспользоваться методами [`expires`](block-js-xscript-cookie-methods.md#expires) и [`permanent`](block-js-xscript-cookie-methods.md#permanent).

Все методы кроме `name` и `value` к существуют в двух вариантах:
 
 
- `xscript.cookie.{property}()` (без параметров) — возвращает текущее значение свойства {property} куки;
- `xscript.cookie.{name}(property_value)` (с параметром) — устанавливает свойство {property} куки в значение property_value.

Исключение составляют методы [`name`](block-js-xscript-cookie-methods.md#name) и [`value`](block-js-xscript-cookie-methods.md#value), которые не могут менять соответствующие значения.

#### `domain([property_value])` {#domain}

Возвращает или устанавливает значение свойства `domain` куки.

#### `expires([property_value])` {#expires}

Возвращает или устанавливает время (в формате Unix time), до которого кука будет валидна.

#### `httpOnly([property_value])` {#httponly}

Возвращает или устанавливает значение свойства `httpOnly` куки .

#### `name()` {#name}

Возвращает имя куки.

#### `path([property_value])` {#path}

Возвращает или устанавливает значение атрибута `path` куки.

#### `permanent([property_value=true|false])` {#permanent}

При вызове `permanent(true)` устанавливает время жизни куки до 2038 года.

При вызове без параметров возвращает значение `true`, если установлено время жизни куки до 2038 года. В противном случае возвращает значение `false`.

#### `secure([property_value=true|false])` {#secure}

Возвращает или устанавливает для куки свойство `secure`.

#### `value()` {#value}
Возвращает значение куки.

**Пример использования**:

```xml
<x:js>
  c = new xscript.cookie('foo', 'bar');
  xscript.print(c.name());
  c.path("/some/path");
  c.domain(".example.com");
  c.secure(true);
  c.expires(123456789);
  //c.permanent(true)
  xscript.print("path:", c.path());
  xscript.response.setCookie(c);
</x:js>
```

В данном примере создаётся кука foo со значением <q>bar</q>, задаются её дополнительные параметры, после чего она отправляется пользователю (`xscript.response.setCookie(c)`).

### Узнайте больше {#learn-more}
* [JavaScript-блок](../concepts/block-js-ov.md)
* [js](../reference/js.md)