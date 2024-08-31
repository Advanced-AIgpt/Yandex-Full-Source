# Объект Request: данные запроса пользователя

## Определение {#definition}

_Yandex::Request_ является одним из базовых объектов XScript и содержит всю информацию о запросе пользователя:

- параметры запроса в строке URL (query);
- куки, которые установлены у пользователя (cookies);
- заголовки HTTP-запроса и HTTP-ответа (request headers и response headers);
- данные, пришедшие в заголовках запроса (POST), как в виде MIME-сообщения, так и в виде его фрагментов.

Структура _RequestData_ является упрощением объекта Request. Она содержит меньше данных: только поля, в которых хранятся куки, заголовки HTTP-запроса, параметры запроса, remote-ip и hostname. RequestData не включает заголовки HTTP-ответа.

Интерфейсы Request и RequestData описаны в [request.idl](https://svn.yandex.ru/wsvn/xscript/xscript-corba/trunk/idl/request.idl).


## Особенности работы {#workdetails}

Объект Request позволяет CORBA-компонентам получать доступ к параметрам, заголовкам, URL и другим данным HTTP-запроса. Это всегда удалённый запрос.

{% note info %}

Заголовки ответа на HTTP-запрос (response headers) могут устанавливаться с помощью тега [\<header\>](../reference/header.md). Если заголовок с одним и тем же именем выставлен и CORBA-методом, и тегом \<header\>, пользователю будет возвращено значение, заданное в \<header\>.

{% endnote %}



## Особенности использования {#usagedetails}

Доступ к объекту Request в XScript осуществляется либо через параметр [объектного типа](parameters-complex-ov.md) Request, либо через RequestData, либо через один из [параметров-адаптеров](parameters-matching-ov.md):

- QueryArg: именованный параметр HTTP-запроса;
- Cookie: заданная кука HTTP-запроса;
- HTTPHeader: заданный заголовок HTTP-запроса.

Кроме того, многие параметры запроса могут быть получены через параметр типа [ProtocolArg](../appendices/protocol-arg.md).


## Время жизни {#lifetime}

Время жизни объекта Request соответствует времени работы с данным HTTP-запросом.


## Пример использования {#example}

```
<block>
   <name>Yandex/Blogs/BloggerAux.id</name>
   <method>LoadAvatar</method>
   <param type="AuthInfo"/>
   <param type="HTTPHeader">host</param>
   <param type="Request"/>
</block>
```


### Узнайте больше {#learn-more}
* [Общий процесс обработки запроса](../concepts/request-handling-ov.md)
* [Все типы параметров методов, вызываемых в XScript-блоках](../appendices/block-param-types.md)
* [Тип параметра ProtocolArg](../appendices/protocol-arg.md)