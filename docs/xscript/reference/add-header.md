# add-headers

Содержит HTTP-заголовки, которые добавляются в ответ на запрос (response headers).

## Содержит {#contains}

[header](header.md) (1 или более)

## Содержится в {#contained-in}

[xscript](xscript.md)

## Атрибуты {#attrs}

Нет.

## Пример {#example}

```
<add-headers>
    <header name="Cache-Control" value="max-age=0, proxy-revalidate"/>
    <header name="Pragma" value="no-cache"/>
</add-headers>
```

* [Общий процесс обработки запроса](../concepts/request-handling-ov.md)
* [http://www.httpheaders.com](http://www.httpheaders.com)
* [http://www.comptechdoc.org/independent/web/http/reference](http://www.comptechdoc.org/independent/web/http/reference)