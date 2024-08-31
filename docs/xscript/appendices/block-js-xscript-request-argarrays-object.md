# Псевдообъект xscript.request.argArrays

Псевдообъект `xscript.request.args` предоставляет доступ к query-параметрам HTTP-запроса.

Параметры передаются в запрос в виде `?param1=value1&param2=value2...`, а их значения содержатся в свойствах псевдообъекта `(xscript.localargs.{param_name`}) в виде массивов. Каждый массив содержит значения одноименных параметров.

Псевдообъект `xscript.request.argArrays` допускает итерацию по всем своим свойствам, значение которых не равно `undefined`, и считывание значений свойств.

**Пример**

Параметры запроса:

```httpget
foo=ololo&foo=&bar=ololo&xxx&abc=1&abc=2&xxx=xxx 
```

Вывод параметров запроса:

```xml
<x:js>xscript.print(JSON.stringify(xscript.request.argArrays))</x:js>
```

Результат:

```xml
<js>{"foo":["ololo",""],"bar":["ololo"],"xxx":["","xxx"],"abc":["1","2"]}</js>
```

### Узнайте больше {#learn-more}
* [JavaScript-блок](../concepts/block-js-ov.md)
* [js](../reference/js.md)
* [Псевдообъект xscript.request.args](../appendices/block-js-xscript-request-args-object.md)