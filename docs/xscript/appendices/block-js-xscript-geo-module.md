# Модуль xscript.geo

В этом модуле реализованы наиболее востребованные функции [Geo-блока](../concepts/block-geo-ov.md).

**Список функций:**

- regionId;
- nativeRegionId
- isIn
- isYandex
- parents
- children
- regionById
- timezone
- linguistics

#### `regionId`

Определение региона по IP. Если IP не указан или пустой, то возвращается текущий регион по куке `yandex_gid`. Аналог [set_state_region](block-geo-methods.md#set_state_region).

**Входные параметры**

- IP-адрес в строковом представлении. Необязательный параметр.

#### `nativeRegionId`

Определение региона без учета информации куки `yandex_gid`. Аналог [get_native_region](block-geo-methods.md#get_native_region).

**Входные параметры**

Отсутствуют.

#### `isIn`

Проверяет вхождение в родительский регион. Аналог [is_in](block-geo-methods.md#is_in). Возвращает `true`, если проверяемый регион входит в родительский, `false`, если не входит. Если родительский регион или проверяемый регион не определились, то возвращается `undefined`.

**Входные параметры**

- Идентификатор родительского региона;
- Идентификатор проверяемого региона. Необязательный параметр. При его отсутствии текущий регион пользователя определяется автоматически.

#### `isYandex`

Проверяет равенство ( `nativeRegionId()==9999` ), где 9999 — это некоторое <q>магическое</q> число, которое содержится внутри гео-блока и может быть в будущем изменено или стать множеством идентификаторов.

**Входные параметры**

Отсутствуют.

#### `parents`

Возвращает текущий и родительский регионы в виде массива числовых идентификаторов. Аналог [set_state_parents](block-geo-methods.md#set_state_parents).

**Входные параметры**

- Идентификатор региона. Необязательный параметр. При его отсутствии текущий регион пользователя определяется автоматически.

#### `children`

Возвращает текущий и дочерние регионы в виде массива числовых идентификаторов.

**Входные параметры**

- Идентификатор региона. Необязательный параметр. При его отсутствии текущий регион пользователя определяется автоматически.

#### `regionById`

Возвращает полную информацию о регионе из геобазы в виде хеша.

**Входные параметры**

- Идентификатор региона.

#### `timezone`

Возвращает имя родительского региона во временной зоне которого находится регион с указанным идентификатором.

**Входные параметры**

- Идентификатор региона.

#### `linguistics`

Возвращает склонения названия региона на указанном языке в виде хеша.

**Входные параметры**

- Идентификатор региона;
- ISO-код языка. ISO-код — это двухбуквенное обозначение стран и территорий, закрепленные в стандарте [ISO 3166](http://www.iso.org/iso/country_codes);

#### Пример:

```
<x:js>
<![CDATA[
    var gid_native = xscript.geo.nativeRegionId();
    var gid_local = xscript.geo.regionId("192.168.1.1");
    var gid_yakuts = xscript.geo.regionId("94.245.156.234");

    xscript.print( 'isIn(225,' + gid_yakuts + '):', xscript.geo.isIn(225, gid_yakuts) );
    xscript.print( 'isYandex:', xscript.geo.isYandex() )

    var parents;
    if (gid_yakuts) parents = xscript.geo.parents(gid_yakuts);
    xscript.print( gid_native, JSON.stringify(parents), typeof(parents), gid_local )

    var children = xscript.geo.children(213);
    for (var i = 0; i < children.length; ++i) {
       xscript.print(children[i]);
    }

    // Россия
    var r_russia = xscript.geo.regionById(225)
    xscript.print( 'r_russia.name:', r_russia.name )

    // Чикаго
    xscript.print( 'regionById(10131):', JSON.stringify(xscript.geo.regionById(10131)) )

    // Прага
    xscript.print( 'timezone(125):', JSON.stringify(xscript.geo.timezone(125)) )

    // Город по умолчанию для казахского языка
    xscript.print( 'linguistics(213, "kk"):', JSON.stringify(xscript.geo.linguistics(213, "kk")) )
]]>
</x:js>
```

Результат:

```xml
<js>
isIn(225,74): true 
isYandex: true 9999 [74,11443,73,225,10001,10000] object undefined 20279 20356 20357 20358 20359 20360 20361 20362 20363 9000 9999 216 
r_russia.name: Россия 
regionById(10131): {"id":10131,"parent":29330,"chief_region":0,"main":false,"name":"Чикаго","ename":"Chicago","short_ename":"","bgp_name":"","synonyms":"","timezone":"America/Chicago","type":6,"position":62817,"phone_code":"312 773 872","zip_code":"","lat":41.808148,"lon":-87.744535,"spn_lat":6.31213,"spn_lon":11.249997,"zoom":7} timezone(125): "Europe/Prague" 
linguistics(213, "kk"): {"nominative":"Мәскеу","genitive":"Мәскеудің","dative":"Мәскеуге","prepositional":"Мәскеуде","preposition":"","locative":"","directional":""}
</js>
```


### Узнайте больше {#learn-more}
* [JavaScript-блок](../concepts/block-js-ov.md)
* [js](../reference/js.md)