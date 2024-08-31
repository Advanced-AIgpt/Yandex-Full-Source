# Методы объекта xscript.request

#### Список методов:

- [getArg](block-lua-request-methods.md#get_arg);
- [getArgs](block-lua-request-methods.md#get_args);
- [getContentEncoding](block-lua-request-methods.md#get_content_encoding);
- [getContentLength](block-lua-request-methods.md#get_content_length);
- [getContentType](block-lua-request-methods.md#get_content_type);
- [getCookie](block-lua-request-methods.md#get_cookie);
- [getHeader](block-lua-request-methods.md#get_header);
- [getHeaders](block-lua-request-methods.md#getheaders);
- [getHost](block-lua-request-methods.md#get_host);
- [getHTTPUser](block-lua-request-methods.md#get_http_user);
- [getMethod](block-lua-request-methods.md#get_method);
- [getOriginalHost](block-lua-request-methods.md#get_original_host);
- [getOriginalURI](block-lua-request-methods.md#get_original_uri);
- [getOriginalUrl](block-lua-request-methods.md#get_original_url);
- [getPath](block-lua-request-methods.md#get_path);
- [getPathInfo](block-lua-request-methods.md#get_path_info);
- [getQuery](block-lua-request-methods.md#get_query);
- [getQueryArgs](block-lua-request-methods.md#get-query-args);
- [getRealPath](block-lua-request-methods.md#get_real_path);
- [getRemoteIp](block-lua-request-methods.md#get_remote_ip);
- [getScriptName](block-lua-request-methods.md#get_script_name);
- [getURI](block-lua-request-methods.md#get_uri);
- [hasArg](block-lua-request-methods.md#has_arg);
- [hasCookie](block-lua-request-methods.md#has_cookie);
- [hasHeader](block-lua-request-methods.md#has_header);
- [isBot](block-lua-request-methods.md#is_bot);
- [isSecure](block-lua-request-methods.md#is_secure).

#### `getArg(name)` {#get_arg}

Получает параметр запроса с именем `name`.

На данный момент поддерживаются только уникальные имена параметров. Так результат вызова

```
xscript.request:getArg('a')
```

для запроса

```
www.url.ru/page?a=1&a=2
```

непредсказуем.

#### `getArgs(name)` {#get_args}

Возвращает все параметры запроса с именем `name` в виде таблицы ([table](http://www.lua.org/pil/2.5.md)).

#### `getCookie(name)` {#get_cookie}

Получает значение куки с именем `name`.

#### `getContentEncoding()` {#get_content_encoding}

Возвращает содержимое строки HTTP-заголовков Content-Encoding.

#### `getContentLength()` {#get_content_length}

Возвращает содержимое строки HTTP-заголовков Content-Length.

#### `getContentType()` {#get_content_type}

Возвращает содержимое строки HTTP-заголовков Content-Type.

#### `getHeader(name)` {#get_header}

Получает значение заголовка HTTP-запроса с именем `name`.

#### `getHeaders()` {#getheaders}

Возвращает все заголовки HTTP-запроса в виде таблицы.

#### `getHost()` {#get_host}

Возвращает содержимое строки HTTP-заголовков Host.

#### `getHTTPUser()` {#get_http_user}

Возвращает аутентификационное имя пользователя.

#### `getMethod()` {#get_method}

Возвращает метод запроса.

#### `getOriginalHost()` {#get_original_host}

Возвращает содержимое строки HTTP-заголовков x-original-host.

#### `getOriginalURI()` {#get_original_uri}

Возвращает содержимое строки HTTP-заголовков x-original-uri.

#### `getOriginalUrl()` {#get_original_url}

Возвращает строку вида "http://" (или "https://") + originalhost + originaluri.

#### `getPath()` {#get_path}

Возвращает виртуальный путь до запрашиваемого ресурса.

#### `getPathInfo()` {#get_path_info}

Возвращает дополнительную информацию о пути до запрашиваемого ресурса.

#### `getQuery()` {#get_query}

Возвращает query запроса (параметры, переданные в составе URL после вопросительного знака) в виде строки.

#### `getQueryArgs()` {#get-query-args}

Возвращает параметры запроса в виде массива таблиц, состоящих из двух строк и двух столбцов. Первым элементом первой строки таблицы является строка "name", вторым - имя параметра. Первым элементом второй строки таблицы является строка "value", вторым - значение параметра. Порядок таблиц в массиве соответствует порядку параметров в запросе.

**Пример**:

В приведенном ниже примере выводятся на печать параметры запроса.

```
<lua>
     args = xscript.request:getQueryArgs()
     for i,v in ipairs(args) do
       print(v.name)
       print(v.value)
     end
</lua>
```

#### `getRealPath()` {#get_real_path}

Возвращает физический путь до запрашиваемого ресурса.

#### `getRemoteIp()` {#get_remote_ip}

Возвращает IP-адрес посетителя.

#### `getScriptName()` {#get_script_name}

Возвращает виртуальный путь к выполняемому скрипту.

#### `getURI()` {#get_uri}

Возвращает URI запроса.

#### `hasArg(name)` {#has_arg}

Проверяет наличие в запросе параметра с именем `name`.

#### `hasCookie(name)` {#has_cookie}

Проверяет наличие куки с именем `name`.

#### `hasHeader(name)` {#has_header}

Проверяет наличие HTTP-заголовка с именем `name`.

#### `isBot()` {#is_bot}

Возвращает значение "true", если запрос идентифицирован как запрос от робота. В противном случае возвращает значение "false".

#### `isSecure()` {#is_secure}

Возвращает значение "true", если запрос сделан по протоколу HTTPS.

**Пример**:

```
<x:lua>
      <![CDATA[
          print("Hello from lua!");
          xscript.state:setString("test args", xscript.request:getArg("query"))
          xscript.state:setString("test headers", xscript.request:getHeader("Host"))
          xscript.state:setString("test cookies", xscript.request:getCookie("SessionId"))
          print("Bye from lua!")
      ]]>
</x:lua>
```

### Узнайте больше {#learn-more}
* [Lua-блок](../concepts/block-lua-ov.md)
* [lua](../reference/lua.md)
