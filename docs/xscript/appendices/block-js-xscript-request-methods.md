# Функции пространства имен xscript.request

Функции для получения информации о HTTP-запросе.

#### Список функций:

- [getContentEncoding](block-js-xscript-request-methods.md#get_content_encoding);
- [getContentLength](block-js-xscript-request-methods.md#get_content_length);
- [getContentType](block-js-xscript-request-methods.md#get_content_type);
- [getDocumentRoot](block-js-xscript-request-methods.md#get_document_root);
- [getHost](block-js-xscript-request-methods.md#get_host);
- [getHTTPUser](block-js-xscript-request-methods.md#get_http_user);
- [getMethod](block-js-xscript-request-methods.md#get_method);
- [getOriginalHost](block-js-xscript-request-methods.md#get_original_host);
- [getOriginalURI](block-js-xscript-request-methods.md#get_original_uri);
- [getOriginalUrl](block-js-xscript-request-methods.md#get_original_url);
- [getPath](block-js-xscript-request-methods.md#get_path);
- [getPathInfo](block-js-xscript-request-methods.md#get_path_info);
- [getQuery](block-js-xscript-request-methods.md#get_query);
- [getRealPath](block-js-xscript-request-methods.md#get_real_path);
- [getRemoteIP](block-js-xscript-request-methods.md#get_remote_ip);
- [getScriptName](block-js-xscript-request-methods.md#get_script_name);
- [getURI](block-js-xscript-request-methods.md#get_uri);
- [isBot](block-js-xscript-request-methods.md#is_bot);
- [isSecure](block-js-xscript-request-methods.md#is_secure).

#### `getContentEncoding()` {#get_content_encoding}
Возвращает содержимое HTTP-заголовка _Content-Encoding_.
#### `getContentLength()` {#get_content_length}
Возвращает содержимое HTTP-заголовка _Content-Length_.
#### `getContentType()` {#get_content_type}
Возвращает содержимое HTTP-заголовка _Content-Type_.
#### `getDocumentRoot()` {#get_document_root}
Возвращает местоположение корневого каталога HTTP-сервера.
#### `getHost()` {#get_host}
Возвращает содержимое HTTP-заголовка _Host_.
#### `getHTTPUser()` {#get_http_user}
Возвращает аутентификационное имя пользователя.
#### `getMethod()` {#get_method}
Возвращает метод HTTP-запроса.
#### `getOriginalHost()` {#get_original_host}
Возвращает содержимое HTTP-заголовка _x-original-host_.
#### `getOriginalURI()` {#get_original_uri}
Возвращает содержимое HTTP-заголовка _x-original-uri_.
#### `getOriginalURL()` {#get_original_url}
Возвращает строку вида <q>http://</q> (или <q>https://</q>) + originalhost + originaluri.
#### `getPath()` {#get_path}
Возвращает относительный путь до запрашиваемого ресурса.
#### `getPathInfo()` {#get_path_info}
Возвращает дополнительную информацию о пути до запрашиваемого ресурса.
#### `getQuery()` {#get_query}
Возвращает query-параметры запроса в виде строки.
#### `getRealPath()` {#get_real_path}
Возвращает абсолютный путь до запрашиваемого ресурса.
#### `getRemoteIP()` {#get_remote_ip}
Возвращает IP-адрес клиента.
#### `getScriptName()` {#get_script_name}
Возвращает виртуальный путь к выполняемому скрипту.
#### `getURI()` {#get_uri}
Возвращает URI запроса.
#### `isBot()` {#is_bot}
Возвращает <q>true</q>, если запрос идентифицирован как запрос от робота. В противном случае возвращает <q>false</q>.
#### `isSecure()` {#is_secure}
Возвращает значение <q>true</q>, если запрос сделан по протоколу HTTPS.

**Пример**:

```xml
<x:js>
  xscript.print( xscript.request.Method() + ' request ' );
  xscript.print( ' to host ' + xscript.request.getHost() );
  xscript.print( ' from IP' + xscript.request.getRemoteIP() );
</x:js>
```

### Узнайте больше {#learn-more}
* [JavaScript-блок](../concepts/block-js-ov.md)
* [js](../reference/js.md)
