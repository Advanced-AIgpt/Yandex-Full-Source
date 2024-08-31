# Как выставить заголовки HTTP-ответа и куки

## Как выставлять заголовки HTTP-ответа {#set-headers}

Существует три способа выставлять заголовки ответа:

* с помощью тега [\<add-headers\>](../reference/add-header.md);

* метод CORBA-блока может напрямую добавлять заголовки в объект [Request](../concepts/request-ov.md), который для этого должен быть передан ему в качестве параметра;

* заголовок _Expires_ можно выставить с помощью атрибута `http-expire-time-delta` тега [\<xscript\>](../reference/xscript.md).

Тег `add-headers` обрабатывается в конце обработки страницы, поэтому если один и тот же заголовок выставляется и в ходе обработки страницы методами [CORBA-блока](../concepts/block-corba-ov.md), и тегом `add-headers`, будет использовано значение, заданное в `add-headers`. Однако если в основном XSL-преобразовании, которое накладывается после обработки `add-headers`, есть вызов CORBA-блока, в котором выставляется такой же заголовок, будет выставлено значение, определенное в этом вызове.

Если заголовок с одним и тем же именем выставлен несколькими методами в одном XML-файле, то будет использовано только последнее значение.


## Как выставлять куки {#SetCookieTask}

Куки можно выставлять следующими способами:

* в [Lua-блоке](../concepts/block-lua-ov.md) с помощью метода [setCookie](../appendices/block-lua-response-methods.md#set_cookie) объекта Response или метода [new](../appendices/block-lua-cookie-methods.md#new) объекта Cookie. В объекте Cookie также реализованы методы для выставления времени жизни куки и её атрибутов domain, path и т.д.;

* добавлять куки в объект [Request](../concepts/request-ov.md) с помощью методов setCookie, setCookieDomain, setCookieDomainPath, setCookieStruct (см. [request.idl](https://svn.yandex.ru/websvn/wsvn/xscript/xscript-corba/trunk/idl/request.idl)). Для работы с этими методами в интерфейс `Request` также добавлена структура `Cookie`.

### Узнайте больше {#learn-more}
* [Приводимые типы параметров](../concepts/parameters-matching-ov.md)