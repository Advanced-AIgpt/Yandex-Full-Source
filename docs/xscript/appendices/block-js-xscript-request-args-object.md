# Псевдообъект xscript.request.args

Псевдообъект `xscript.request.args` предоставляет доступ к query-параметрам HTTP-запроса.

Параметры передаются в запрос в виде `?param1=value1&param2=value2...`, а их значения содержатся в свойствах псевдообъекта `(xscript.localargs.{param_name`}).

Если в запрос передано несколько параметров с одинаковым именем, то соответствующее свойство будет содержать значение первого параметра. Для считывания значений одноименных параметров, можно воспользоваться псевдообъектом [xscript.request.argArrays](block-js-xscript-request-argarrays-object.md).

Псевдообъект `xscript.request.args` допускает итерацию по всем своим свойствам, значение которых не равно `undefined`, и считывание значений свойств.

**Пример**

Параметры запроса:

```httpget
foo=ololo&foo=&bar=ololo&xxx&abc=1&abc=2&xxx=xxx 
```

Вывод параметров запроса:

```xml
<x:js>xscript.print(JSON.stringify(xscript.request.args))</x:js>
```

Результат:

```xml
<js>{"foo":"ololo","bar":"ololo","xxx":"","abc":"1"}</js>
```

### Узнайте больше {#learn-more}
* [JavaScript-блок](../concepts/block-js-ov.md)
* [js](../reference/js.md)
* [Псевдообъект xscript.request.argArrays](../appendices/block-js-xscript-request-argarrays-object.md)