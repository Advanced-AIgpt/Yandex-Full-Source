# HTTP-блок 

_HTTP-блок_ предназначен для выполнения запросов к удаленным серверам по протоколу HTTP(S).

HTTP-блок может выполняться асинхронно (если установлен атрибут [threaded="yes"](../appendices/attrs-ov.md#threaded)), а результаты его работы могут [кэшироваться](block-results-caching.md).

Входные параметры методов HTTP-блока могут [конкатенироваться](../appendices/block-http-methods.md).

Во всех методах блока можно выставлять исходящие HTTP-заголовки с помощью тега [header](../reference/header.md).

**Пример HTTP-блока**:

```
<http>
     <method>getHttp</method>
     <param type="String">http://devel.yandex.ru/index.xml?id=1</param>
</http>
```

В приведенном примере производится запрос контента с указанного адреса с помощью метода HTTP-блока [`get_http`](../appendices/block-http-methods.md#get_http).

### Узнайте больше {#learn-more}
* [Методы HTTP-блока](../appendices/block-http-methods.md)
* [http](../reference/http.md)
* [Метаинформация HTTP-блока](./meta.md#http)
