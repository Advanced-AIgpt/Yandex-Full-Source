# Тип параметра ProtocolArg

`ProtocolArg` - тип параметра, позволяющий использовать переменные окружения протокола FastCGI и содержимое ряда HTTP-заголовков в качестве параметров XScript.

Пример объявления параметра типа `ProtocolArg`:
 
```
<param type="ProtocolArg">originalhost</param>
```

В качестве содержимого параметра `ProtocolArg` можно использовать одну из следующих переменных:
 
- `bot`{#bot} - флаг, указывающий, что запрос был сделан роботом. Возможные значения: "yes", "no";
- `content-encoding` - содержимое строки HTTP-заголовков Content-Encoding;
- `content-length` - содержимое строки HTTP-заголовков Content-Length;
- `content-type` - содержимое строки HTTP-заголовков Content-Type;
- `host` - содержимое строки HTTP-заголовков Host;
- `http_user` - содержимое переменной REMOTE_USER. Аутентифицированное имя пользователя;
- `method` - содержимое переменной REQUEST_METHOD. Метод запроса;
- `originalhost` - содержимое строки HTTP-заголовков x-original-host. В случае его отсутствия используется host;
- `originaluri` {#originaluri} - содержимое строки HTTP-заголовков x-original-uri. В случае его отсутствия используется значение переменной окружения REQUEST_URI;
- `originalurl` - строка вида `"http://" (или "https://") + originalhost + originaluri`;
- `path` - содержимое переменной SCRIPT_NAME. Виртуальный путь до запрашиваемого ресурса;
- `pathinfo` - содержимое переменной PATH_INFO;
- `port` - номер порта, на который пришел запрос от пользователя;
- `query` - содержимое переменной QUERY_STRING. Параметры запроса, переданные в составе URL после вопросительного знака;
- `realpath` - содержимое переменной SCRIPT_FILENAME. Физический путь до запрашиваемого ресурса;
- `remote_ip`{#remote_ip} - содержимое строки HTTP-заголовков x-real-ip. В случае его отсутствия - содержимое переменной REMOTE_ADDR. IP-адрес клиента;
- `secure` - флаг, указывающий, что используется протокол HTTPS. Возможные значения: "yes", "no";
- `uri` - строка вида `path + pathinfo + "?" + query`.

Кроме того, полный список указанных выше параметров можно получить вызовом метода Mist-блока `set_state_by_protocol`:

```
<mist>
    <method>set_state_by_protocol</method>
    <param type="String"/>
</mist>
```

### Узнайте больше {#learn-more}
* [Приводимые типы параметров](../concepts/parameters-matching-ov.md)