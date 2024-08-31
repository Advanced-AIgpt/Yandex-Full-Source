# Модуль xscript.cache

**Список функций:**

- save;
- load;
- loadData.

#### `save`

Кэширует объект в оперативной памяти.

**Входные параметры**:

- Идентификатор объекта;
- Объект. Он может быть как простым, так и сложным и не должен содержать функции и `undefined` поля;
- Время кэширования в секундах. Значение должно быть больше 5, иначе функция вернет `false`.

#### `load`

Возвращает ранее сохраненный объект.

**Входные параметры:**

- Идентификатор объекта, кэшированного функцией `save`.

#### `loadData`

Возвращает сложный объект с ключами `key` (идентификатор объекта), `value` (объект в том виде, в котором он был сохранен функцией `save`) и `expired` (конечное время хранения объекта в памяти).

**Входные параметры:**

- Идентификатор объекта, кэшированного функцией `save`.

#### Пример:

```
<!-- xscript.cache.load -->
<x:js>
    var key = 'hello_object'
    xscript.print( JSON.stringify( xscript.cache.load(key) ) )
</x:js>

<!-- xscript.cache.loadData -->
<x:js>
    var data = xscript.cache.loadData(key)
    xscript.print( JSON.stringify(data) )
</x:js>

<!-- xscript.cache.save if not loaded in previous (cache_time=5) -->
<x:js>
    if (!data) {
        var obj = {'message': 'Hello, world!'}
        var cache_time = 5
        xscript.print( xscript.cache.save(key, obj, cache_time) )
    }
</x:js>
```

При первом обращении к этой странице функции `load` и `loadData` вернут `undefined`. При обновлении страницы функции вернут кэшированный объект. Через 5 секунд опять будет возвращен `undefined`.

```
<page xmlns:x="http://www.yandex.ru/xscript">
<!-- xscript.cache.load -->
<js>
    undefined
</js>
<!-- xscript.cache.loadData -->
<js>
    undefined
</js>
<!-- xscript.cache.save if not loaded in previous (cache_time=5) -->
<js>
    true
</js>
</page>
```

```xml
<page xmlns:x="http://www.yandex.ru/xscript">
<!-- xscript.cache.load -->
<js>
    {"message":"Hello, world!"}
</js>
<!-- xscript.cache.loadData -->
<js>
    {"key":"hello_object","value":{"message":"Hello, world!"},"expired":1330513252}
</js>
<!-- xscript.cache.save if not loaded in previous (cache_time=5) -->
<js/>
</page>
```

{% note info %}

Работа с превдообъектом xscript.cache возможна только при подключенном модуле [xscript-memcache.so](../concepts/modules.md#xscript-memcache)

{% endnote %}


### Узнайте больше {#learn-more}
* [JavaScript-блок](../concepts/block-js-ov.md)
* [js](../reference/js.md)