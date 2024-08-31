# Псевдообъект xscript.params

Псевдообъект `xscript.params` предоставляет доступ к параметрам JavaScript-блока.

Параметр передается в блок в виде `<param type="{param_type}" id="{param_name}">{param_value}</param>`, а его значение содержится в свойстве `xscript.params.{param_name`}.

Псевдообъект `xscript.params` допускает итерацию по всем своим свойствам, значение которых не равно `undefined`, и считывание значений свойств.

```xml
<x:js>
  <param type="String" id="my_params_str">My parameters</param>
  <param type="QueryArg" id="q">query</param>
  <param type="QueryArg" id="xxx"/>
  <param type="StateArg" id="sss"/>
  <param type="StateArg" id="nnn"/>
  <param type="UID" id="uid"/>
  <param type="Login" id="login"/>
  
  xscript.print(xscript.params.my_params_str + ':');  
  xscript.print(JSON.stringify(xscript.params));
</x:js>
```

### Узнайте больше {#learn-more}
* [JavaScript-блок](../concepts/block-js-ov.md)
* [js](../reference/js.md)

