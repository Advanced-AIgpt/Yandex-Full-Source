# Модуль xscript.langdetect

**Список функций:**

- find;
- list;
- findDomain;
- cookie2language;
- language2cookie.

#### `find`

Определяет язык отображения страницы.

**Входные параметры**:

- `geo` — js-список регионов из геобазы (результат `xscript.geo.parents()`) или comma-separated строка с регионами (результат `mist:setStateParents`);
- `langfilter` — js-список или comma-separated строка с перечнем языков.

**Возвращаемое значение:**

Объект следующего вида: `{"id":"uk","name","Ua"}`.

#### `list`

Создает список релевантных пользователю языков.

**Входные параметры**:

- `geo` — js-список регионов из геобазы (результат `xscript.geo.parents()`) или comma-separated строка с регионами (результат `mist:setStateParents`);
- `langfilter` — js-список или comma-separated строка с перечнем языков.

**Возвращаемое значение:**

Список объектов следующего вида: `[{"id":"ru","name":"Ru"},{"id":"uk","name":"Ua"}]`.

#### `findDomain`

Определяет домен и регион пользователя.

**Входные параметры**:

- `geo` — js-список регионов из геобазы (результат `xscript.geo.parents()`) или comma-separated строка с регионами (результат `mist:setStateParents`);
- `domainfilter` — js-список или comma-separated строка с перечнем доменов.

**Возвращаемое значение:**

Объект следующего вида: `{"domain":"bayonet.yandex.ru","changed":false,"contentRegion":9999}`.

#### `cookie2language`

Преобразует числовой идентификатор языка в строковый.

**Входные параметры**:

- `cookie_val` — целочисленный идентификатор языка из куки my.

**Возвращаемое значение:**

Строка с идентификатором языка или `undefined`.

#### `language2cookie`

Преобразует строковый идентификатор языка в числовой.

**Входные параметры**:

- `lang_id` — строковый идентификатор языка

**Возвращаемое значение:**

Целочисленный идентификатор языка или `undefined`.

#### Пример:

```
<!-- Borislav geo.parents(24896) -->
<x:js>
    var borislav_parents = xscript.geo.parents(24896)
    xscript.print( 'Borislav geo-list:', JSON.stringify(borislav_parents))
</x:js>

<!-- x:js find & list by geo parents(24896) Borislav, custom filter = ['uk', 'ru', 'kk', 'be'] -->
<x:js>
    var custom_filter = ['uk', 'ru', 'kk', 'be']
    xscript.print( 'Find: ', JSON.stringify( xscript.langdetect.find(borislav_parents, custom_filter) ) )
</x:js>
<x:js>
    xscript.print( 'List: ', JSON.stringify( xscript.langdetect.list(borislav_parents, custom_filter) ) )
</x:js>

<!-- findDomain for current region -->
<x:js>
    var geo = xscript.geo.parents()
    xscript.print( 'Current region: ' , JSON.stringify(geo) )
</x:js>
<x:js>xscript.print( 'findDomain : ' , JSON.stringify( xscript.langdetect.findDomain(geo, ['ru','ua']) ) )</x:js>

<!-- cookie2language, language2cookie -->
<x:js> xscript.print( 'cookie2language(2): ', xscript.langdetect.cookie2language(2) ) </x:js>
<x:js> xscript.print( 'language2cookie("kk"): ', xscript.langdetect.language2cookie("kk") ) </x:js>

```

В результате обращения из московского офиса Яндекса получим следующий результат:

```xml
<page xmlns:x="http://www.yandex.ru/xscript">
<js>
    Borislav geo-list: [24896,20529,20524,187,166,10001,10000]
</js>
<!-- x:js find & list by geo parents(24896) Borislav, custom filter = ['uk', 'ru', 'kk', 'be'] -->
<js>
    Find: {"id":"ru","name":"Ru"}
</js>
<js>
    List: [{"id":"ru","name":"Ru"},{"id":"uk","name":"Ua"}]
</js>
<!-- findDomain for current region -->
<js>
    Current region: [9999,213,1,3,225,10001,10000]
</js>
<js>
    findDomain : {"domain":"bayonet.user.graymantle.yandex.ru","changed":false,"contentRegion":9999}
</js>
<!-- cookie2language, language2cookie -->
<js>
    cookie2language(2): uk
</js>
<js>
    language2cookie("kk"): 4
</js>
</page>
```

### Узнайте больше {#learn-more}
* [JavaScript-блок](../concepts/block-js-ov.md)
* [js](../reference/js.md)