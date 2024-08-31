# Ответ

Ответ — сообщение сервера на запрос клиента.

## Пример ответа {#response-example}

```
HTTP/1.1 204 No Content
Content-Type: application/json;charset=utf-8
Server: Jetty(9.1.5.v20140505)
```

В этом примере:
- `HTTP/1.1 204 No Content` — стартовая строка с кодом ответа 204;
- `Content-Type`, `Server` — HTTP-заголовки;
- тела ответа нет.

## Стартовая строка {#start}

Стартовая строка HTTP-ответа содержит:

1. Информацию о версии протокола — `HTTP/1.1`.
2. Код состояния (англ. _status codes_), который определяет реакцию сервера на запрос — `204`.

    {% cut "Коды ответов" %}
    
    Коды сгруппированы в 5 классов:
    
    - Информационные `100`–`199`.
    - Успешные `200`–`299`.
    - Перенаправления `300`–`399`.
    - Клиентские ошибки `400`–`499`.
    - Серверные ошибки `500`–`599`.
    
    В таблице приведены коды, которые чаще всего будут встречаться в работе. Список всех стандартных кодов см. в статье [Список кодов состояния HTTP](https://ru.wikipedia.org/wiki/Список_кодов_состояния_HTTP#Обзорный_список).
    
    Код и пояснение | Описание
    ----- | -----
    **200 OK** | Ответ на успешные GET, PUT, PATCH или DELETE. Используется также для POST, который не приводит к созданию.
    **201 Created** | Ответ на POST, который приводит к созданию.
    **204 No Content** | Ответ на успешный запрос, в котором возвращены только заголовки без тела сообщения (например, запрос DELETE).
    **304 Not Modified** | Документ не изменялся с определенного времени. Используйте этот код состояния, когда заголовки HTTP-кеширования находятся в работе.
    **400 Bad Request** | В запросе содержится синтаксическая ошибка, тело запроса не может быть проанализировано.
    **401 Unauthorized** | Не указаны или недействительны данные аутентификации. Активируйте всплывающее окно **auth**, если приложение используется из браузера.
    **403 Forbidden** | Аутентификация прошла успешно, но аутентифицированный пользователь не имеет доступа к ресурсу.
    **404 Not found** | Запрашивается несуществующий ресурс.
    **405 Method Not Allowed** | Запрашивается HTTP-метод, который не разрешен для аутентифицированного пользователя.
    **410 Gone** | Ресурс в этой конечной точке (англ. Endpoint) больше не доступен. Полезно в качестве защитного ответа для старых версий API.
    **415 Unsupported Media Type** | В качестве части запроса был указан неправильный тип содержимого.
    **422 Unprocessable Entity** | Используется для проверки ошибок.
    **429 Too Many Requests** | Слишком много запросов за короткое время, поэтому запрос отклоняется.

    {% endcut %}
    
3. Пояснение (англ. _reason phrase_) к коду ответа для пользователя (необязательная часть) — `No Content`.

## Заголовки {#headers}

{% include [parameters-headers-definition](../_includes/api-theory/request/id-parameters/headers-definition.md) %}


```
Content-Type: application/json;charset=utf-8
Server: Jetty(9.1.5.v20140505)
```

{% include [parameters-header-type](../_includes/api-theory/request/id-parameters/header-type.md) %}


- Основные заголовки (англ. _general headers_), например, `Via (en-US)`, относящиеся к сообщению в целом.
- Заголовки ответа (англ. _response headers_), например, `Vary` и `Accept-Ranges`, сообщающие дополнительную информацию о сервере, которая не уместилась в строку состояния.
- Заголовки сущности (англ. _entity headers_), например `Content-Length`, относящиеся к телу сообщения. Они отсутствуют, если у запроса нет тела.

## Тело ответа {#body}

{% include [parameters-body-definition](../_includes/api-theory/request/id-parameters/body-definition.md) %}


Пример:

```
Date: Wed, 02 Jun 2021 00:41:41 GMT
Content-Language: en
Last-Modified: Tue, 25 May 2021 15:08:46 GMT
Content-Type: text/html; charset=UTF-8
Content-Encoding: gzip
...
Accept-Ranges: bytes
Content-Length: 27606
Connection: keep-alive

<!DOCTYPE html>
<html class="client-nojs" lang="en" dir="ltr">
<head>
<meta charset="UTF-8">
<title>Web server - Wikipedia</title>
...
```

Тело — необязательная часть ответа, у ответов с кодом состояния, например, `201` или `204`, оно обычно отсутствует.

Тела можно разделить на три категории:

- Одноресурсные тела (англ. _single-resource bodies_), состоящие из отдельного файла известной длины, определяемые двумя заголовками: `Content-Type` и `Content-Length`.
- Одноресурсные тела (англ. _single-resource bodies_), состоящие из отдельного файла неизвестной длины, разбитого на небольшие части (chunks) с заголовком `Transfer-Encoding (en-US)`, значением которого является `chunked`.
- Многоресурсные тела (англ. _multiple-resource bodies_), состоящие из множества частей, каждая из которых содержит свой сегмент информации.
