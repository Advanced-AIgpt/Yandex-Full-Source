# Псевдообъект xscript.localargs

Псевдообъект `xscript.localargs` предоставляет доступ к параметрам [Local-блока](../concepts/block-local-ov.md), содержащего данный JavaScript-блок.

Параметр передается в блок в виде `<param type="{param_type}" id="{param_name}">{param_value}</param>`, а его значение содержится в свойстве `xscript.localargs.{param_name`}.

Псевдообъект `xscript.localargs` допускает итерацию по всем своим свойствам, значение которых не равно `undefined`, и считывание значений свойств.

```xml
<x:js id="localargs">
  xscript.print('defined keys of xscript.localargs:');
    for (var key in xscript.localargs) {
      xscript.print(key, ': ', xscript.localargs[key]);
    }
</x:js>
```

### Узнайте больше {#learn-more}
* [JavaScript-блок](../concepts/block-js-ov.md)
* [js](../reference/js.md)