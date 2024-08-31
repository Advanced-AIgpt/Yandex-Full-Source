# Методы объекта xscript.response

#### Список методов:

- [redirectToPath](block-lua-response-methods.md#redirect_to_path);
- [setCookie](block-lua-response-methods.md#set_cookie);
- [setContentType](block-lua-response-methods.md#set_content_type);
- [setExpireTimeDelta](block-lua-response-methods.md#set_expire_time_delta);
- [setHeader](block-lua-response-methods.md#set_header);
- [setStatus](block-lua-response-methods.md#set_status);
- [write](block-lua-response-methods.md#write).

#### `redirectToPath(path)` {#redirect_to_path}
Выполняет редирект на адрес `path`, т.е., в сущности, делает следующее: 
```
xscript.response:setStatus(302)
xscript.response:setHeader('Location', path)
```

#### `setContentType(type)` {#set_content_type}
Устанавливает значение `type` для HTTP-заголовка `Content-type`, т.е., в сущности, делает следующее: 
```
xscript.response:setHeader('Content-type', type)
```

#### `setCookie(cookie)` {#set_cookie}
Устанавливает пользователю новую куку `cookie` типа [xscript.cookie](block-lua-cookie-methods.md).

#### `setExpireTimeDelta(time)` {#set_expire_time_delta}

Устанавливает время хранения документа в кэше браузера, формируя HTTP-заголовок Expires (аналогично атрибуту `http-expire-time-delta` тега [\<xscript\>](../reference/xscript.md)).

Принимает на вход время кэширования в секундах.

**Пример**:

```
<x:lua>
     <![CDATA[
         xscript.response:setExpireTimeDelta(600);
     ]]>
</x:lua>
```

#### `setHeader(name, value)` {#set_header}

Вставляет в ответ заголовок с именем `name` и значением `value`. Например: 
```
xscript.response:setHeader('X-Bases-Belong-To', 'US!')
```

#### `setStatus(status)` {#set_status}

Вставляет в ответ статус `status`. Формат описан в [rfc2616](http://www.ietf.org/rfc/rfc2616.txt).

#### `write(str)` {#write}

Формирует HTTP-ответ <q>с нуля</q>. Записывает в выходной поток HTTP-статус, исходящие HTTP-заголовки (однократно, при первом вызове функции), затем помещает строку `str` в тело ответа. При этом основной XSL-шаблон не накладывается.

Функция недоступна в блоках, в которых отключено проксирование данных (`<x:local proxy="request|no">`).

После первого вызова функции становятся недоступны все операции с HTTP-ответом, кроме записи в выходной поток с помощью функции write.

В случае невозможности записи в выходной поток, возвращает false.

```
<x:lua>
<![CDATA[
 xscript.response:write('<html><body>Raw output</body></html>')
]]>
</x:lua>
```

**Пример использования**:

```
xscript.response:setStatus(200)
if check_martians_attack() then
   xscript.response:redirectToPath("teleport://alpha.centaur/")
end
```

При атаке марсиан пользователь увидит на экране следующее сообщение:

```
302 Found
Location: teleport://aplha.centaur/
```

Ещё один пример:

```
<x:lua>
      <![CDATA[
          xscript.response:setStatus(404)
          xscript.response:setHeader("X-Header", "Foo Bar")
          xscript.response:setContentType("application/binary")
      ]]>
</x:lua>
```

### Узнайте больше {#learn-more}
* [Lua-блок](../concepts/block-lua-ov.md)
* [lua](../reference/lua.md)
