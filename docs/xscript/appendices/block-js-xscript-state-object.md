# Псевдообъект xscript.state

Псевдообъект `xscript.state` предоставляет доступ к [контейнеру State](../concepts/state-ov.md) и позволяет считывать, изменять и удалять данные контейнера.

Имена и значения свойств псевдообъекта соответствуют именам и значениям заголовков запроса.

Псевдообъект `xscript.state` допускает итерацию по всем своим свойствам, значение которых не равно `undefined`, считывание и изменение значений свойств, а также их удаление. Операция над свойством псевдообъекста равносильна аналогичной операции над одноименным элементом контейнера State.

```xml
<x:js>
  xscript.print ('old check state.str: ', xscript.state.str || "undef");
  xscript.state.str = "changed value";
  xscript.print ('new check state.str: ', xscript.state.str);
  delete xscript.state.str;
  xscript.print ('del check state.str: ', xscript.state.str || "undef");
  for (var key in xscript.state) {
    var v = xscript.state[key];
    if (v !== undefined) {
      xscript.print(key, v);
    }
  }
</x:js>
```

Для удаления свойства объекта можно присвоить ему значение null.

```xml
<x:js>
  xscript.state.str = "value";
  xscript.state.str = null;
</x:js>
<x:mist method="dumpState"/>
```

{% note info %}

**Для удаления свойства рекомендуется использовать оператор `delete`.**

{% endnote %}

С помощью `xscript.state` можно размещать в контейнере State сложные объекты. При этом действуют следующие ограничения:

- Ключи, значение которых равно `undefined`, будут проигнорированы.
- Функция в качестве значения свойства недопустима. Попытка добавить в State такой объект приведет к выдаче ошибки.

### Узнайте больше {#learn-more}
* [JavaScript-блок](../concepts/block-js-ov.md)
* [js](../reference/js.md)