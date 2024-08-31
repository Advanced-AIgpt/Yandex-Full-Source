# Псевдообъект xscript.request.headers

Псевдообъект `xscript.request.headers` предоставляет доступ к заголовкам HTTP-запроса.

Имена и значения свойств псевдообъекта соответствуют именам и значениям заголовков запроса.

Псевдообъект `xscript.request.headers` допускает итерацию по всем своим свойствам, значение которых не равно `undefined`, и считывание значений свойств.

```xml
<x:js>
    xscript.print('defined keys of xscript.request.headers:')
    xscript.print(JSON.stringify(xscript.request.headers))
</x:js>
```

### Узнайте больше {#learn-more}
* [JavaScript-блок](../concepts/block-js-ov.md)
* [js](../reference/js.md)