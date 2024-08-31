# Функции пространства имен xscript.response

#### Список методов:

- [redirectToPath](block-js-xscript-response-methods.md#redirect_to_path);
- [setCookie](block-js-xscript-response-methods.md#set_cookie);
- [setContentType](block-js-xscript-response-methods.md#set_content_type);
- [setExpireTimeDelta](block-js-xscript-response-methods.md#set_expire_time_delta);
- [setHeader](block-js-xscript-response-methods.md#set_header);
- [setStatus](block-js-xscript-response-methods.md#set_status);
- [write](block-js-xscript-response-methods.md#write).

#### `redirectToPath(path)` {#redirect_to_path}

Выполняет редирект на адрес `path`, т.е., в сущности, делает следующее: 
```javascript
xscript.response.setStatus(302);
xscript.response.setHeader('Location', path);
```

#### `setContentType(type)` {#set_content_type}

Устанавливает для HTTP-заголовка `Content-type` значение `type`, т.е., в сущности, делает следующее: 
```javascript
xscript.response.setHeader('Content-type', type);
```

#### `setCookie(cookie)` {#set_cookie}
Устанавливает пользователю новую куку. Параметр cookie представляет собой объект класса [xscript.cookie](block-js-xscript-cookie-methods.md) .
#### `setExpireTimeDelta(time)` {#set_expire_time_delta}

Устанавливает время хранения документа в кэше браузера, формируя HTTP-заголовок _Expires_ (аналогично атрибуту `http-expire-time-delta` тега [\<xscript\>](../reference/xscript.md)).

Принимает на вход время кэширования в секундах.

**Пример**:

```javascript
xscript.response.setExpireTimeDelta(600);
```

#### `setHeader(name, value)` {#set_header}

Добавляет в HTTP-ответ заголовок с именем `name` и значением `value`. Например: 
```javascript
xscript.response.setHeader('X-Bases-Belong-To', 'US!');
```

#### `setStatus(status)` {#set_status}

Устанавливает статус HTTP-ответа в `status`. Формат описан в [rfc2616](http://www.ietf.org/rfc/rfc2616.txt).

#### `write(str)` {#write}

Формирует HTTP-ответ <q>с нуля</q>. Записывает в выходной поток HTTP-статус, исходящие HTTP-заголовки (однократно, при первом вызове функции), затем помещает строку `str` в тело ответа. При этом основной XSL-шаблон не накладывается.

Функция недоступна в блоках, в которых отключено проксирование данных (`<x:local proxy="request|no">`).

После первого вызова функции становятся недоступны все операции с HTTP-ответом, кроме записи в выходной поток с помощью функции `write`.

В случае невозможности записи в выходной поток, возвращает `false`.

```
<x:js>
<![CDATA[
 xscript.response.write('<html><body>Raw output</body></html>');
]]>
</x:js>
```

**Пример использования**:

```javascript
xscript.response.setStatus(200);
if (is_mars_attacks()){
  xscript.response.redirectToPath("teleport://alpha.centaur/")
}
```

При атаке марсиан пользователь увидит на экране следующее сообщение:

```no-highlight
302 Found
Location: teleport://aplha.centaur/
```

### Узнайте больше {#learn-more}
* [JavaScript-блок](../concepts/block-js-ov.md)
* [js](../reference/js.md)